#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "LexiconBuilder.hpp"
#include "TrieNode.hpp"
#include "Suggestion.hpp"

class AutoComplete {
private:
    std::unique_ptr<TrieNode> root;
    size_t total_words;
    size_t max_suggestions;
    bool case_sensitive;
    size_t min_prefix_length;
    
    // Helper functions
    void collect_suggestions(TrieNode* node, 
                           std::vector<Suggestion>& results,
                           int max_results) const;
    
    TrieNode* find_prefix_node(const std::string& prefix) const;
    
    std::string normalize_text(const std::string& text) const;
    
    void insert_word(const std::string& word, uint32_t frequency);
    
    // Advanced ranking
    double calculate_relevance(const std::string& word, 
                              const std::string& prefix,
                              uint32_t frequency) const;

public:
    // Constructor
    AutoComplete(size_t max_suggestions = 10, 
                bool case_sensitive = false,
                size_t min_prefix_length = 2);
    
    // Initialize from lexicon
    bool initialize_from_lexicon(const LexiconBuilder* lexicon);
    
    // Initialize from word list
    bool initialize_from_words(const std::vector<std::pair<std::string, uint32_t>>& words);
    
    // Get suggestions for a prefix
    std::vector<Suggestion> get_suggestions(const std::string& prefix) const;
    
    // Get suggestions with custom limit
    std::vector<Suggestion> get_suggestions(const std::string& prefix, 
                                           size_t limit) const;
    
    // Check if word exists
    bool word_exists(const std::string& word) const;
    
    // Check if prefix is valid
    bool is_valid_prefix(const std::string& prefix) const;
    
    // Get word frequency
    uint32_t get_word_frequency(const std::string& word) const;
    
    // Configuration setters
    void set_max_suggestions(size_t max) { max_suggestions = max; }
    void set_case_sensitive(bool sensitive) { case_sensitive = sensitive; }
    void set_min_prefix_length(size_t length) { min_prefix_length = length; }
    
    // Configuration getters
    size_t get_max_suggestions() const { return max_suggestions; }
    bool is_case_sensitive() const { return case_sensitive; }
    size_t get_min_prefix_length() const { return min_prefix_length; }
    
    // Statistics
    size_t get_vocabulary_size() const { return total_words; }
    void print_statistics() const;
    
    // Clear all data
    void clear();
    
    // Batch processing - get suggestions for multiple prefixes
    std::unordered_map<std::string, std::vector<Suggestion>> 
    batch_suggestions(const std::vector<std::string>& prefixes) const;
    
    // Fuzzy matching (for typo tolerance)
    std::vector<Suggestion> fuzzy_search(const std::string& query, int max_edit_distance = 2) const;

    bool save_to_binary(const std::string& file_path) const;
    bool load_from_binary(const std::string& file_path);


};
