#pragma once
#include "LexiconBuilder.hpp"
#include "ForwardIndex.hpp"
#include "InvertedIndex.hpp"
#include "TextPreProcessor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>

// Structure to hold search results
struct SearchResult {
    std::string doc_id;           // Paper ID
    std::string title;            // Paper title
    std::string abstract;         // Paper abstract
    double score;                 // BM25 relevance score
    uint32_t doc_internal_id;     // Internal document ID
    std::unordered_map<std::string, uint32_t> matched_terms; // Matched query terms and their frequencies
    
    // For sorting results by score (descending)
    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

class SearchEngine {
protected:
    // Core components
    LexiconBuilder* lexicon;
    ForwardIndex* forward_index;
    InvertedIndex* inverted_index;
    TextPreprocessor* preprocessor;
    
    // Reverse lexicon for word_id -> word mapping
    std::unordered_map<uint32_t, std::string> reverse_lexicon;
    
    // Document statistics for BM25
    double average_doc_length;
    uint32_t total_documents;
    
    // BM25 tuning parameters (standard values)
    const double k1 = 1.5;      // Term frequency saturation parameter
    const double b = 0.75;       // Document length normalization parameter
    
public:
    // Constructor
    SearchEngine(LexiconBuilder* lex, 
                 ForwardIndex* fwd_idx, 
                 InvertedIndex* inv_idx,
                 TextPreprocessor* prep);
    
    // Main search function - returns top_k results for a query
    virtual std::vector<SearchResult> search(const std::string& query, int top_k = 10);
    
    // Helper: Get document details by doc_id
    const DocumentIndex* get_document(const std::string& doc_id) const;
    
    // Print search results in a formatted way
    void print_results(const std::vector<SearchResult>& results) const;
    
private:
    // Calculate BM25 score for a single document
    double calculate_bm25(uint32_t doc_internal_id,
                         const std::string& doc_id_str,
                         const std::vector<uint32_t>& query_word_ids,
                         const std::unordered_map<uint32_t, uint32_t>& query_term_freq);
    
    double calculate_bm25_optimized(
    const std::string& doc_id_str,
    const std::vector<uint32_t>& query_word_ids,
    const std::unordered_map<uint32_t, uint32_t>& query_term_freq,
    const std::unordered_map<uint32_t, double>& idf_cache);
    
    double calculate_idf(uint32_t word_id, uint32_t doc_frequency);
    
};