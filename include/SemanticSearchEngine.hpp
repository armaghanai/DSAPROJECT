#include "WordEmbeddingsEngine.hpp"
#include "SearchEngine.hpp"
#include "SemanticSearchResult.hpp"

class SemanticSearchEngine {
protected:
    WordEmbeddingsEngine* embedding_engine;
    SearchEngine* bm25_engine;
    
    // Weights for hybrid search
    float semantic_weight = 0.6f;  // 60% semantic, 40% BM25
    float bm25_weight = 0.4f;
    
    // Cache for document embeddings (doc_id -> embedding)
    std::unordered_map<std::string, std::vector<float>> doc_embedding_cache;

    
public:
    SemanticSearchEngine(WordEmbeddingsEngine* emb_engine, SearchEngine* bm25_eng);
    
    // Set hybrid search weights
    void set_weights(float semantic_w, float bm25_w);
    
    // Pure semantic search using word embeddings
    virtual std::vector<SemanticSearchResult> semantic_search(const std::string& query,const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,int top_k = 10);
    
    // Hybrid search: BM25 + Semantic
    std::vector<SemanticSearchResult> hybrid_search(const std::string& query,const std::vector<std::string>& query_tokens,
    const ForwardIndex* forward_index,int top_k = 10);
    
    
    // Print semantic search results
    void print_semantic_results(const std::vector<SemanticSearchResult>& results) const;

protected:
            std::vector<float> get_document_embedding(const std::string& doc_id,const DocumentIndex& doc_index);

};