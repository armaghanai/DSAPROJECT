#pragma once
#include "SearchEngine.hpp"
#include "QueryLemmatizer.hpp"
#include <unordered_map>
#include <memory>

class CachedSearchEngine : public SearchEngine {
private:
    static constexpr size_t CACHE_SIZE = 1000;
    std::unordered_map<std::string, std::vector<SearchResult>> result_cache;
    QueryLemmatizer lemmatizer;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    std::string normalize_query(const std::string& query);
    
public:
    CachedSearchEngine(LexiconBuilder* lex, 
                      ForwardIndex* fwd_idx, 
                      InvertedIndex* inv_idx,
                      TextPreprocessor* prep);
    
    // Override search with caching
    std::vector<SearchResult> search(const std::string& query, int top_k) override;
    
    // Clear cache
    void clear_cache();
    
    // Get cache statistics
    void print_cache_stats() const;
};