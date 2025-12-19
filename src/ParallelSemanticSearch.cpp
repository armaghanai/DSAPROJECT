#include "../include/ParallelSemanticSearch.hpp"
#include <iostream>
#include <algorithm>
#include <vector>

std::vector<SemanticSearchResult> ParallelSemanticSearch::semantic_search(
    const std::string& query,
    const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,
    int top_k) {
    
    // ✅ Get BM25 candidates first (fast, ~100ms)
    std::cout << "[Parallel Semantic] Getting BM25 candidates..." << std::endl;
    auto bm25_candidates = bm25_engine->search(query, top_k * 10);  // Get more candidates
    
    if (bm25_candidates.empty()) {
        return {};
    }
    
    // Get query embedding
    std::vector<float> query_embedding = 
        embedding_engine->get_average_embedding(query_tokens);
    
    if (query_embedding.empty()) {
        return {};
    }
    
    // ✅ Parallel score ONLY the candidates
    std::vector<std::vector<SemanticSearchResult>> thread_results(num_threads);
    size_t candidates_per_thread = (bm25_candidates.size() + num_threads - 1) / num_threads;
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; t++) {
        size_t start_idx = t * candidates_per_thread;
        size_t end_idx = std::min(start_idx + candidates_per_thread, bm25_candidates.size());
        
        threads.emplace_back([this, &query_embedding, &bm25_candidates, &thread_results, 
                             t, start_idx, end_idx, forward_index]() {
            for (size_t i = start_idx; i < end_idx; i++) {
                const auto& candidate = bm25_candidates[i];
                const DocumentIndex* doc = forward_index->get_document(candidate.doc_id);
                
                if (!doc) continue;
                
                std::vector<float> doc_embedding = get_document_embedding(candidate.doc_id, *doc);
                
                if (!doc_embedding.empty()) {
                    float score = embedding_engine->cosine_similarity(query_embedding, doc_embedding);
                    
                    if (score > 0.0f) {
                        SemanticSearchResult result;
                        result.doc_id = candidate.doc_id;
                        result.title = doc->title;
                        result.abstract = doc->abstract_text;
                        result.semantic_score = score;
                        result.combined_score = score;
                        
                        thread_results[t].push_back(result);
                    }
                }
            }
        });
    }
    
    // Wait for threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Merge and sort
    std::vector<SemanticSearchResult> results;
    for (const auto& tr : thread_results) {
        results.insert(results.end(), tr.begin(), tr.end());
    }
    
    std::sort(results.begin(), results.end(),
             [](const SemanticSearchResult& a, const SemanticSearchResult& b) {
                 return a.semantic_score > b.semantic_score;
             });
    
    if (results.size() > static_cast<size_t>(top_k)) {
        results.resize(top_k);
    }
    
    return results;
}