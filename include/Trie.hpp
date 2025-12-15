#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "TrieNode.hpp"


class Trie {
private:
    std::unique_ptr<TrieNode> root;
    size_t total_words;
    
    // Helper function for recursive traversal
    void collect_words(TrieNode* node, 
                      std::vector<std::pair<std::string, uint32_t>>& results,
                      int max_results) const;
    
    // Helper function for prefix search
    TrieNode* find_prefix_node(const std::string& prefix) const;

public:
    Trie();
    
    // Insert a word into the Trie with its frequency
    void insert(const std::string& word, uint32_t frequency = 1);
    
    // Search for exact word match
    bool search(const std::string& word) const;
    
    // Check if any word starts with given prefix
    bool starts_with(const std::string& prefix) const;
    
    // Get auto-complete suggestions for a prefix
    // Returns list of (word, frequency) pairs sorted by frequency
    std::vector<std::pair<std::string, uint32_t>> 
    autocomplete(const std::string& prefix, int max_results = 10) const;
    
    // Build Trie from lexicon data
    void build_from_lexicon(
        const std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& lexicon_data);
    
    // Get statistics
    size_t get_word_count() const { return total_words; }
    
    // Clear the Trie
    void clear();
    
    // Get word frequency
    uint32_t get_frequency(const std::string& word) const;
    
    // Print statistics
    void print_statistics() const;
};
