// SemanticSearchResult.hpp
struct SemanticSearchResult {
    std::string doc_id;
    std::string title;
    std::string abstract;
    float semantic_score;      // Cosine similarity (0-1)
    float bm25_score;         // BM25 score from keyword search
    float combined_score;     // Weighted combination
    std::vector<std::string> matched_terms;
    
    // For sorting
    bool operator<(const SemanticSearchResult& other) const {
        return combined_score > other.combined_score;
    }
};