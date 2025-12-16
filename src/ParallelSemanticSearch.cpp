#include "ParallelSemanticSearch.hpp"
#include <iostream>
#include <algorithm>
#include <vector>

ParallelSemanticSearch::ParallelSemanticSearch(WordEmbeddingsEngine* emb_engine,
                                             SearchEngine* bm25_eng,
                                             size_t threads)
    : SemanticSearchEngine(emb_engine, bm25_eng) {
    num_threads = (threads == 0) ? std::thread::hardware_concurrency() : threads;
    std::cout << "\n=== Parallel Semantic Search Initialized ===" << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "=============================================\n" << std::endl;
}

std::vector<SemanticSearchResult> ParallelSemanticSearch::semantic_search(
    const std::string& query,
    const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,
    int top_k) {
    
    std::vector<SemanticSearchResult> results;
    std::vector<std::vector<SemanticSearchResult>> thread_results(num_threads);
    
    // Get query embedding
    std::cout << "[Semantic] Computing query embedding..." << std::endl;
    std::vector<float> query_embedding = 
        embedding_engine->get_average_embedding(query_tokens);
    
    if (query_embedding.empty()) {
        return results;
    }
    
    auto all_docs = forward_index->get_forward_index();
    size_t docs_per_thread = (all_docs.size() + num_threads - 1) / num_threads;
    
    // Create thread pool
    std::vector<std::thread> threads;
    auto doc_it = all_docs.begin();
    
    for (size_t t = 0; t < num_threads && doc_it != all_docs.end(); t++) {
        threads.emplace_back([this, &query_embedding, &all_docs, &thread_results, 
                             t, docs_per_thread, doc_it]() mutable {
            size_t count = 0;
            while (doc_it != all_docs.end() && count < docs_per_thread) {
                const auto& [doc_id, doc_index] = *doc_it;
                
                std::vector<float> doc_embedding = 
                    get_document_embedding(doc_id, doc_index);
                
                if (!doc_embedding.empty()) {
                    float score = embedding_engine->cosine_similarity(
                        query_embedding, doc_embedding);
                    
                    if (score > 0.0f) {
                        SemanticSearchResult result;
                        result.doc_id = doc_id;
                        result.title = doc_index.title;
                        result.abstract = doc_index.abstract_text;
                        result.semantic_score = score;
                        result.combined_score = score;
                        
                        thread_results[t].push_back(result);
                    }
                }
                
                ++doc_it;
                ++count;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Merge results from all threads
    for (const auto& thread_result : thread_results) {
        results.insert(results.end(), thread_result.begin(), thread_result.end());
    }
    
    // Sort and filter
    std::sort(results.begin(), results.end(),
             [](const SemanticSearchResult& a, const SemanticSearchResult& b) {
                 return a.semantic_score > b.semantic_score;
             });
    
    if (results.size() > static_cast<size_t>(top_k)) {
        results.resize(top_k);
    }
    
    std::cout << "[Semantic] Found " << results.size() 
              << " results (parallel, " << num_threads << " threads)" << std::endl;
    
    return results;
}