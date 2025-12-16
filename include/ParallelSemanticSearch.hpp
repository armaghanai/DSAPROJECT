#pragma once
#include "SemanticSearchEngine.hpp"
#include <thread>
#include <mutex>
#include <vector>

class ParallelSemanticSearch : public SemanticSearchEngine {
private:
    size_t num_threads;
    std::mutex result_mutex;
    
public:
    ParallelSemanticSearch(WordEmbeddingsEngine* emb_engine,
                          SearchEngine* bm25_eng,
                          size_t threads = 0);
    
    // Override semantic search with parallelization
    std::vector<SemanticSearchResult> semantic_search(
        const std::string& query,
        const std::vector<std::string>& query_tokens,
        const ForwardIndex* forward_index,
        int top_k) override;
};
