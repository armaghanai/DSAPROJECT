#pragma once
#include "SearchEngine.hpp"
#include "Lemmatizer.hpp"
#include <unordered_map>

class CachedSearchEngine : public SearchEngine {
private:
    std::unordered_map<std::string, std::vector<SearchResult>> result_cache;
    Lemmatizer* lemmatizer;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    static constexpr size_t CACHE_SIZE = 1000;

public:
    // ✅ DECLARATION ONLY (no body)
    CachedSearchEngine(LexiconBuilder* lex,
                       ForwardIndex* fwd_idx,
                       InvertedIndex* inv_idx,
                       TextPreprocessor* prep,
                       Lemmatizer* lemma);

    std::string normalize_query(const std::string& query);
    std::vector<SearchResult> search(const std::string& query, int top_k) override;
    void clear_cache();
    void print_cache_stats() const;
};
