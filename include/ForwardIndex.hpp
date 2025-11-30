#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>

// Simple structure to hold word_id and frequency (no positions)
struct TermPosting {
    uint32_t word_id;
    uint32_t frequency;
    
    TermPosting() : word_id(0), frequency(0) {}
    TermPosting(uint32_t id, uint32_t freq) : word_id(id), frequency(freq) {}
};

// Structure representing a single document in the forward index
struct DocumentIndex {
    std::string doc_id;           // Paper ID (cord_uid)
    std::string title;            // Document title
    std::string abstract_text;    // Abstract text
    uint32_t doc_length;          // Total number of terms in document
    std::vector<TermPosting> terms; // All terms in this document
    
    DocumentIndex() : doc_length(0) {}
};

class ForwardIndex {
private:
    // Maps document_id (cord_uid) -> document index data
    std::unordered_map<std::string, DocumentIndex> forward_index;
    
    // Maps doc_id -> internal numeric document ID
    std::unordered_map<std::string, uint32_t> doc_id_map;
    uint32_t next_doc_id;
    
    // Statistics
    uint32_t total_documents;
    uint64_t total_terms;
    
public:
    ForwardIndex();
    
    // Add a document to the forward index
    void add_document(const std::string& doc_id,
                     const std::string& title,
                     const std::string& abstract_text,
                     const std::vector<uint32_t>& word_ids);
    
    // Get document index by doc_id
    const DocumentIndex* get_document(const std::string& doc_id) const;
    
    // Get all terms for a specific document
    const std::vector<TermPosting>* get_document_terms(const std::string& doc_id) const;
    
    // Get document length
    uint32_t get_document_length(const std::string& doc_id) const;
    
    // Get term frequency in a specific document
    uint32_t get_term_frequency(const std::string& doc_id, uint32_t word_id) const;
    
    // Get statistics
    uint32_t get_total_documents() const { return total_documents; }
    uint32_t get_total_terms() const { return total_terms; }
    size_t get_index_size() const { return forward_index.size(); }
    
    // Save forward index to binary file
    bool save_to_binary(const std::string& file_path);
    std::unordered_map<std::string, uint32_t> get_doc_id_map();
    
    // Load forward index from binary file
    bool load_from_binary(const std::string& file_path);
    
    // Save forward index to CSV (human-readable format)
    void save_to_csv(const std::string& file_path);
    void save_first_n_to_csv(const std::string& file_path, size_t number_of_docs);
    
    // Clear the index
    void clear();
    
    // Print statistics
    void print_statistics() const;
    
    // Get average document length
    double get_average_doc_length() const;
};

