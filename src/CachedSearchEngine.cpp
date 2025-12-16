#include "../include/CachedSearchEngine.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

CachedSearchEngine::CachedSearchEngine(LexiconBuilder* lex, 
                                     ForwardIndex* fwd_idx, 
                                     InvertedIndex* inv_idx,
                                     TextPreprocessor* prep)
    : SearchEngine(lex, fwd_idx, inv_idx, prep) {
    std::cout << "\n=== Cached Search Engine Initialized ===" << std::endl;
    std::cout << "Query cache size: " << CACHE_SIZE << std::endl;
    std::cout << "Lemmatizer enabled" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

std::string CachedSearchEngine::normalize_query(const std::string& query) {
    // Lemmatize the query for better matching
    std::vector<std::string> tokens = preprocessor->preprocess(query);
    std::vector<std::string> lemmatized = lemmatizer.lemmatize_tokens(tokens);
    
    std::string normalized;
    for (const auto& token : lemmatized) {
        normalized += token + " ";
    }
    return normalized;
}

std::vector<SearchResult> CachedSearchEngine::search(const std::string& query, int top_k) {
    std::string cache_key = normalize_query(query);
    
    // Check cache
    auto cache_it = result_cache.find(cache_key);
    if (cache_it != result_cache.end()) {
        cache_hits++;
        std::cout << "[CACHE HIT] Query: " << query << std::endl;
        
        // Return cached results (or subset if top_k is different)
        auto results = cache_it->second;
        if (results.size() > static_cast<size_t>(top_k)) {
            results.resize(top_k);
        }
        return results;
    }
    
    // Cache miss - perform search
    cache_misses++;
    auto results = SearchEngine::search(query, top_k);
    
    // Store in cache (limit cache size)
    if (result_cache.size() < CACHE_SIZE) {
        result_cache[cache_key] = results;
    }
    
    return results;
}

void CachedSearchEngine::clear_cache() {
    result_cache.clear();
    cache_hits = 0;
    cache_misses = 0;
    std::cout << "Cache cleared" << std::endl;
}

void CachedSearchEngine::print_cache_stats() const {
    std::cout << "\n=== Cache Statistics ===" << std::endl;
    std::cout << "Cache size: " << result_cache.size() << " / " << CACHE_SIZE << std::endl;
    std::cout << "Cache hits: " << cache_hits << std::endl;
    std::cout << "Cache misses: " << cache_misses << std::endl;
    
    if (cache_hits + cache_misses > 0) {
        double hit_rate = (static_cast<double>(cache_hits) / (cache_hits + cache_misses)) * 100.0;
        std::cout << "Hit rate: " << std::fixed << std::setprecision(2) << hit_rate << "%" << std::endl;
    }
    std::cout << "=======================\n" << std::endl;
}