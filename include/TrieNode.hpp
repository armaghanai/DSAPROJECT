#pragma once
#include <unordered_map>
#include <cstdint>
#include <string>
#include <memory>

// TrieNode represents each node in the Trie
struct TrieNode {
    //used a smart pointer for efficient memomry management
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool is_end_of_word;
    std::string word;  // Store complete word at end nodes
    uint32_t frequency; // Word frequency for ranking suggestions
    
    TrieNode() : is_end_of_word(false), frequency(0) {}
};

