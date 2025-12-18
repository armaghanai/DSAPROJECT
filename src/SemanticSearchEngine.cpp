#include "../include/SemanticSearchEngine.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

std::vector<float> SemanticSearchEngine::get_document_embedding(
    const std::string& doc_id,
    const DocumentIndex& doc_index) {
    
    // Check cache first
    auto it = doc_embedding_cache.find(doc_id);
    if (it != doc_embedding_cache.end()) {
        return it->second;
    }
    
    // Extract words from document (abstract + title)
    std::string full_text = doc_index.title + " " + doc_index.abstract_text;
    
    // Simple tokenization
    std::vector<std::string> tokens;
    std::istringstream iss(full_text);
    std::string token;
    while (iss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        // Remove non-alphanumeric
        token.erase(
            std::remove_if(token.begin(), token.end(),
                          [](char c) { return !std::isalnum(c); }),
            token.end());
        
        if (!token.empty() && token.length() >= 2) {
            tokens.push_back(token);
        }
    }
    
    // Get average embedding
    std::vector<float> embedding = 
        embedding_engine->get_average_embedding(tokens);
    
    // Cache it
    doc_embedding_cache[doc_id] = embedding;
    
    return embedding;
}

SemanticSearchEngine::SemanticSearchEngine(WordEmbeddingsEngine* emb_engine, SearchEngine* bm25_eng)
    : embedding_engine(emb_engine), bm25_engine(bm25_eng) {
    std::cout << "\n=== Semantic Search Engine Initialized ===" << std::endl;
    std::cout << "Embedding dimension: " << embedding_engine->get_dimension() << std::endl;
    std::cout << "Vocabulary size: " << embedding_engine->get_vocabulary_size() << std::endl;
    std::cout << "========================================\n" << std::endl;
}

// Set hybrid search weights
void SemanticSearchEngine::set_weights(float semantic_w, float bm25_w) {
    float total = semantic_w + bm25_w;
    semantic_weight = semantic_w / total;
    bm25_weight = bm25_w / total;
}

// ✅ CRITICAL FIX: Pure semantic search using CANDIDATES ONLY
std::vector<SemanticSearchResult> SemanticSearchEngine::semantic_search(
    const std::string& query,
    const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,
    int top_k) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<SemanticSearchResult> results;
    
    // Get query embedding
    std::cout << "[Semantic] Computing query embedding..." << std::endl;
    std::vector<float> query_embedding = 
        embedding_engine->get_average_embedding(query_tokens);
    
    if (query_embedding.empty() || 
        std::all_of(query_embedding.begin(), query_embedding.end(),
                   [](float f) { return f == 0.0f; })) {
        std::cout << "Warning: Query embedding is zero (words not in vocabulary)" << std::endl;
        return results;
    }
    
    // ✅ KEY OPTIMIZATION: Get candidates from BM25 first!
    std::cout << "[Semantic] Getting BM25 candidates..." << std::endl;
    auto candidate_start = std::chrono::high_resolution_clock::now();
    
    // Get MORE candidates than needed (top_k * 5) for better recall
    auto bm25_candidates = bm25_engine->search(query, top_k * 5);
    
    auto candidate_end = std::chrono::high_resolution_clock::now();
    auto candidate_time = std::chrono::duration_cast<std::chrono::milliseconds>(candidate_end - candidate_start);
    
    std::cout << "[Semantic] Got " << bm25_candidates.size() 
              << " candidates in " << candidate_time.count() << " ms" << std::endl;
    
    if (bm25_candidates.empty()) {
        std::cout << "[Semantic] No candidates found!" << std::endl;
        return results;
    }
    
    // ✅ Score ONLY the candidates (not all documents!)
    std::cout << "[Semantic] Scoring " << bm25_candidates.size() 
              << " candidates (NOT all docs)..." << std::endl;
    auto scoring_start = std::chrono::high_resolution_clock::now();
    
    for (const auto& candidate : bm25_candidates) {
        const DocumentIndex* doc = forward_index->get_document(candidate.doc_id);
        if (!doc) continue;
        
        // Get document embedding (cached or computed)
        std::vector<float> doc_embedding = get_document_embedding(candidate.doc_id, *doc);
        
        if (doc_embedding.empty()) continue;
        
        // Compute cosine similarity
        float semantic_score = embedding_engine->cosine_similarity(
            query_embedding, doc_embedding);
        
        if (semantic_score > 0.0f) {
            SemanticSearchResult result;
            result.doc_id = candidate.doc_id;
            result.title = doc->title;
            result.abstract = doc->abstract_text;
            result.semantic_score = semantic_score;
            result.bm25_score = 0.0f;
            result.combined_score = semantic_score;
            
            results.push_back(result);
        }
    }
    
    auto scoring_end = std::chrono::high_resolution_clock::now();
    auto scoring_time = std::chrono::duration_cast<std::chrono::milliseconds>(scoring_end - scoring_start);
    std::cout << "[Semantic] Scoring time: " << scoring_time.count() << " ms" << std::endl;
    
    // Sort by semantic score
    std::sort(results.begin(), results.end(),
             [](const SemanticSearchResult& a, const SemanticSearchResult& b) {
                 return a.semantic_score > b.semantic_score;
             });
    
    // Keep top K
    if (results.size() > static_cast<size_t>(top_k)) {
        results.resize(top_k);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "[*** SEMANTIC SEARCH TOTAL: " << total_time.count() << " ms ***]" << std::endl;
    std::cout << "[Semantic] Returning " << results.size() 
              << " semantically similar documents\n" << std::endl;
    
    return results;
}

// Hybrid search: BM25 + Semantic
std::vector<SemanticSearchResult> SemanticSearchEngine::hybrid_search(
    const std::string& query,
    const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,
    int top_k) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "\n[Hybrid] Starting hybrid search..." << std::endl;
    
    // Get BM25 results (more documents for re-ranking)
    std::cout << "[Hybrid] Step 1: BM25 search..." << std::endl;
    auto bm25_start = std::chrono::high_resolution_clock::now();
    auto bm25_results = bm25_engine->search(query, top_k * 3);
    auto bm25_end = std::chrono::high_resolution_clock::now();
    auto bm25_time = std::chrono::duration_cast<std::chrono::milliseconds>(bm25_end - bm25_start);
    std::cout << "[Hybrid] BM25 time: " << bm25_time.count() << " ms" << std::endl;
    
    // Get semantic results (using candidates only!)
    std::cout << "[Hybrid] Step 2: Semantic search..." << std::endl;
    auto semantic_start = std::chrono::high_resolution_clock::now();
    auto semantic_results = semantic_search(query, query_tokens, forward_index, top_k * 3);
    auto semantic_end = std::chrono::high_resolution_clock::now();
    auto semantic_time = std::chrono::duration_cast<std::chrono::milliseconds>(semantic_end - semantic_start);
    std::cout << "[Hybrid] Semantic time: " << semantic_time.count() << " ms" << std::endl;
    
    // Combine results
    std::cout << "[Hybrid] Step 3: Combining results..." << std::endl;
    auto combine_start = std::chrono::high_resolution_clock::now();
    
    std::unordered_map<std::string, SemanticSearchResult> combined;
    
    // Add BM25 results
    float max_bm25 = bm25_results.empty() ? 1.0f : bm25_results[0].score;
    if (max_bm25 < 0.01f) max_bm25 = 1.0f;
    
    for (const auto& result : bm25_results) {
        SemanticSearchResult sr;
        sr.doc_id = result.doc_id;
        sr.title = result.title;
        sr.abstract = result.abstract;
        sr.bm25_score = result.score / max_bm25;  // Normalize to 0-1
        sr.semantic_score = 0.0f;
        
        // Copy matched terms from SearchResult to SemanticSearchResult
        for (const auto& [term, freq] : result.matched_terms) {
            sr.matched_terms.push_back(term);
        }
        
        combined[result.doc_id] = sr;
    }
    
    // Add semantic results (update combined or add new)
    float max_semantic = semantic_results.empty() ? 1.0f : semantic_results[0].semantic_score;
    if (max_semantic < 0.01f) max_semantic = 1.0f;
    
    for (const auto& result : semantic_results) {
        float norm_semantic = result.semantic_score / max_semantic;
        
        auto it = combined.find(result.doc_id);
        if (it != combined.end()) {
            // Update existing entry
            it->second.semantic_score = norm_semantic;
        } else {
            // Add new entry
            SemanticSearchResult sr;
            sr.doc_id = result.doc_id;
            sr.title = result.title;
            sr.abstract = result.abstract;
            sr.semantic_score = norm_semantic;
            sr.bm25_score = 0.0f;
            combined[result.doc_id] = sr;
        }
    }
    
    // Calculate combined scores
    std::vector<SemanticSearchResult> final_results;
    for (auto& [doc_id, result] : combined) {
        result.combined_score = (semantic_weight * result.semantic_score) +
                               (bm25_weight * result.bm25_score);
        final_results.push_back(result);
    }
    
    // Sort by combined score
    std::sort(final_results.begin(), final_results.end());
    
    // Keep top K
    if (final_results.size() > static_cast<size_t>(top_k)) {
        final_results.resize(top_k);
    }
    
    auto combine_end = std::chrono::high_resolution_clock::now();
    auto combine_time = std::chrono::duration_cast<std::chrono::milliseconds>(combine_end - combine_start);
    std::cout << "[Hybrid] Combining time: " << combine_time.count() << " ms" << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "[*** HYBRID TOTAL TIME: " << total_time.count() << " ms ***]" << std::endl;
    std::cout << "[Hybrid] Returning " << final_results.size() << " results\n" << std::endl;
    
    return final_results;
}

// Print semantic search results
void SemanticSearchEngine::print_semantic_results(const std::vector<SemanticSearchResult>& results) const {
    if (results.empty()) {
        std::cout << "\nNo results found." << std::endl;
        return;
    }
    
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << "SEMANTIC SEARCH RESULTS (" << results.size() << " documents)" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    for (size_t i = 0; i < results.size(); i++) {
        const auto& result = results[i];
        
        std::cout << "\n[" << (i + 1) << "] ";
        std::cout << "Semantic: " << std::fixed << std::setprecision(4) 
                  << result.semantic_score;
        
        if (result.bm25_score > 0.0f) {
            std::cout << " | BM25: " << result.bm25_score 
                      << " | Combined: " << result.combined_score;
        }
        std::cout << std::endl;
        
        std::cout << std::string(100, '-') << std::endl;
        std::cout << "Title: " << result.title << std::endl;
        std::cout << "Doc ID: " << result.doc_id << std::endl;
        
        std::string abstract = result.abstract;
        if (abstract.length() > 250) {
            abstract = abstract.substr(0, 250) + "...";
        }
        std::cout << "Abstract: " << abstract << std::endl;
    }
    
    std::cout << "\n" << std::string(100, '=') << std::endl;
}