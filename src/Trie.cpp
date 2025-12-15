#include "../include/Trie.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <cstdint>


Trie::Trie() : root(std::make_unique<TrieNode>()), total_words(0) {}

void Trie::insert(const std::string& word, uint32_t frequency) {
    if (word.empty()) return;
    
    TrieNode* current = root.get();
    
    // Convert to lowercase for case-insensitive search
    std::string lower_word = word;
    std::transform(lower_word.begin(), lower_word.end(), 
                  lower_word.begin(), ::tolower);
    
    // Traverse or create nodes for each character
    for (char c : lower_word) {
        if (current->children.find(c) == current->children.end()) {
            current->children[c] = std::make_unique<TrieNode>();
        }
        current = current->children[c].get();
    }
    
    // Mark end of word and store data
    if (!current->is_end_of_word) {
        total_words++;
    }
    
    current->is_end_of_word = true;
    current->word = lower_word;
    current->frequency = frequency;
}

bool Trie::search(const std::string& word) const {
    if (word.empty()) return false;
    
    std::string lower_word = word;
    std::transform(lower_word.begin(), lower_word.end(), 
                  lower_word.begin(), ::tolower);
    
    TrieNode* current = root.get();
    
    for (char c : lower_word) {
        if (current->children.find(c) == current->children.end()) {
            return false;
        }
        current = current->children[c].get();
    }
    
    return current->is_end_of_word;
}

bool Trie::starts_with(const std::string& prefix) const {
    if (prefix.empty()) return true;
    
    std::string lower_prefix = prefix;
    std::transform(lower_prefix.begin(), lower_prefix.end(), 
                  lower_prefix.begin(), ::tolower);
    
    TrieNode* current = root.get();
    
    for (char c : lower_prefix) {
        if (current->children.find(c) == current->children.end()) {
            return false;
        }
        current = current->children[c].get();
    }
    
    return true;
}

TrieNode* Trie::find_prefix_node(const std::string& prefix) const {
    if (prefix.empty()) return root.get();
    
    std::string lower_prefix = prefix;
    std::transform(lower_prefix.begin(), lower_prefix.end(), 
                  lower_prefix.begin(), ::tolower);
    
    TrieNode* current = root.get();
    
    for (char c : lower_prefix) {
        if (current->children.find(c) == current->children.end()) {
            return nullptr;
        }
        current = current->children[c].get();
    }
    
    return current;
}

void Trie::collect_words(TrieNode* node, 
                        std::vector<std::pair<std::string, uint32_t>>& results,
                        int max_results) const {
    if (!node || results.size() >= static_cast<size_t>(max_results)) {
        return;
    }
    
    // If this node marks end of word, add it to results
    if (node->is_end_of_word) {
        results.push_back({node->word, node->frequency});
    }
    
    // Recursively collect from all children
    for (auto& [c, child] : node->children) {
        if (results.size() >= static_cast<size_t>(max_results)) {
            break;
        }
        collect_words(child.get(), results, max_results);
    }
}

std::vector<std::pair<std::string, uint32_t>> 
Trie::autocomplete(const std::string& prefix, int max_results) const {
    std::vector<std::pair<std::string, uint32_t>> suggestions;
    
    if (prefix.empty()) {
        return suggestions;
    }
    
    // Find the node corresponding to the prefix
    TrieNode* prefix_node = find_prefix_node(prefix);
    
    if (!prefix_node) {
        return suggestions; // Prefix not found
    }
    
    // If prefix itself is a word, include it
    if (prefix_node->is_end_of_word) {
        suggestions.push_back({prefix_node->word, prefix_node->frequency});
    }
    
    // Collect all words with this prefix
    collect_words(prefix_node, suggestions, max_results * 2);
    
    // Sort by frequency (descending) then alphabetically
    std::sort(suggestions.begin(), suggestions.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) {
                      return a.second > b.second; // Higher frequency first
                  }
                  return a.first < b.first; // Alphabetical for same frequency
              });
    
    // Limit to max_results
    if (suggestions.size() > static_cast<size_t>(max_results)) {
        suggestions.resize(max_results);
    }
    
    return suggestions;
}

void Trie::build_from_lexicon(
    const std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& lexicon_data) {
    
    std::cout << "\n=== Building Trie from Lexicon ===" << std::endl;
    std::cout << "Total words to insert: " << lexicon_data.size() << std::endl;
    
    int count = 0;
    for (const auto& [word, details] : lexicon_data) {
        uint32_t frequency = details.second; // Get frequency from lexicon
        insert(word, frequency);
        
        count++;
        if (count % 5000 == 0) {
            std::cout << "  Inserted " << count << "/" << lexicon_data.size() 
                     << " words..." << std::endl;
        }
    }
    
    std::cout << "✓ Trie built successfully with " << total_words << " words" << std::endl;
    std::cout << "================================\n" << std::endl;
}

uint32_t Trie::get_frequency(const std::string& word) const {
    if (word.empty()) return 0;
    
    std::string lower_word = word;
    std::transform(lower_word.begin(), lower_word.end(), 
                  lower_word.begin(), ::tolower);
    
    TrieNode* current = root.get();
    
    for (char c : lower_word) {
        if (current->children.find(c) == current->children.end()) {
            return 0;
        }
        current = current->children[c].get();
    }
    
    return current->is_end_of_word ? current->frequency : 0;
}

void Trie::clear() {
    root = std::make_unique<TrieNode>();
    total_words = 0;
}

void Trie::print_statistics() const {
    std::cout << "\n=== Trie Statistics ===" << std::endl;
    std::cout << "Total words: " << total_words << std::endl;
    std::cout << "Root children: " << root->children.size() << std::endl;
    std::cout << "======================\n" << std::endl;
}