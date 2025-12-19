#include "../include/CachedSearchEngine.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

CachedSearchEngine::CachedSearchEngine(
    LexiconBuilder* lex,
    ForwardIndex* fwd_idx,
    InvertedIndex* inv_idx,
    TextPreprocessor* prep,
    Lemmatizer* lemma
)
    : SearchEngine(lex, fwd_idx, inv_idx, prep),
      lemmatizer(lemma)
{
    std::cout << "\n=== Cached Search Engine Initialized ===" << std::endl;
    std::cout << "Query cache size: " << CACHE_SIZE << std::endl;
    std::cout << "Lemmatizer enabled (shared)" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

std::string CachedSearchEngine::normalize_query(const std::string& query) {
    // Tokenize + normalize using shared lemmatizer
    std::vector<std::string> tokens = preprocessor->preprocess(query);

    std::string normalized;
    normalized.reserve(query.size());

    for (const auto& token : tokens) {
        std::string lemma = lemmatizer->lemmatize_word(token);
        if (!lemma.empty()) {
            normalized += lemma;
            normalized += ' ';
        }
    }

    if (!normalized.empty()) {
        normalized.pop_back(); // remove trailing space
    }

    return normalized;
}

std::vector<SearchResult> CachedSearchEngine::search(
    const std::string& query,
    int top_k
) {
    const std::string cache_key = normalize_query(query);

    // Cache hit
    auto it = result_cache.find(cache_key);
    if (it != result_cache.end()) {
        ++cache_hits;

        auto results = it->second;
        if (results.size() > static_cast<size_t>(top_k)) {
            results.resize(top_k);
        }

        return results;
    }

    // Cache miss
    ++cache_misses;
    auto results = SearchEngine::search(query, top_k);

    // Insert into cache (bounded size)
    if (result_cache.size() < CACHE_SIZE) {
        result_cache.emplace(cache_key, results);
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
    std::cout << "Cache size: " << result_cache.size()
              << " / " << CACHE_SIZE << std::endl;
    std::cout << "Cache hits: " << cache_hits << std::endl;
    std::cout << "Cache misses: " << cache_misses << std::endl;

    if (cache_hits + cache_misses > 0) {
        double hit_rate =
            (static_cast<double>(cache_hits) /
            (cache_hits + cache_misses)) * 100.0;

        std::cout << "Hit rate: "
                  << std::fixed << std::setprecision(2)
                  << hit_rate << "%" << std::endl;
    }

    std::cout << "=======================\n" << std::endl;
}
