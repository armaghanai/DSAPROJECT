#include "../include/AutoComplete.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <functional>
#include <fstream>
#include <queue>

AutoComplete::AutoComplete(size_t max_suggestions, 
                          bool case_sensitive,
                          size_t min_prefix_length)
    : root(std::make_unique<TrieNode>()),
      total_words(0),
      max_suggestions(max_suggestions),
      case_sensitive(case_sensitive),
      min_prefix_length(min_prefix_length) {}

std::string AutoComplete::normalize_text(const std::string& text) const {
    if (case_sensitive) {
        return text;
    }
    
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), 
                  normalized.begin(), ::tolower);
    return normalized;
}

void AutoComplete::insert_word(const std::string& word, uint32_t frequency) {
    if (word.empty()) return;
    
    std::string normalized = normalize_text(word);
    TrieNode* current = root.get();
    
    // Traverse or create nodes for each character
    for (char c : normalized) {
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
    current->word = normalized;
    current->frequency = frequency;
}

bool AutoComplete::initialize_from_lexicon(const LexiconBuilder* lexicon) {
    if (!lexicon) {
        std::cerr << "Error: Null lexicon pointer" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Initializing AutoComplete from Lexicon ===" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    clear();
    
    auto lexicon_data = lexicon->get_lexicon_data();
    std::cout << "Processing " << lexicon_data.size() << " words..." << std::endl;
    
    int count = 0;
    for (const auto& [word, details] : lexicon_data) {
        uint32_t frequency = details.second;
        insert_word(word, frequency);
        
        count++;
        if (count % 10000 == 0) {
            std::cout << "  Progress: " << count << "/" << lexicon_data.size() 
                     << " words processed" << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "✓ AutoComplete initialized successfully!" << std::endl;
    std::cout << "  Total words: " << total_words << std::endl;
    std::cout << "  Build time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Max suggestions: " << max_suggestions << std::endl;
    std::cout << "  Min prefix length: " << min_prefix_length << std::endl;
    std::cout << "=========================================\n" << std::endl;
    
    return true;
}

bool AutoComplete::initialize_from_words(
    const std::vector<std::pair<std::string, uint32_t>>& words) {
    
    std::cout << "\n=== Initializing AutoComplete ===" << std::endl;
    std::cout << "Loading " << words.size() << " words..." << std::endl;
    
    clear();
    
    for (const auto& [word, frequency] : words) {
        insert_word(word, frequency);
    }
    
    std::cout << "✓ Loaded " << total_words << " words" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    return total_words > 0;
}

TrieNode* AutoComplete::find_prefix_node(const std::string& prefix) const {
    if (prefix.empty()) return root.get();
    
    std::string normalized = normalize_text(prefix);
    TrieNode* current = root.get();
    
    for (char c : normalized) {
        if (current->children.find(c) == current->children.end()) {
            return nullptr;
        }
        current = current->children[c].get();
    }
    
    return current;
}

void AutoComplete::collect_suggestions(TrieNode* node, 
                                      std::vector<Suggestion>& results,
                                      int max_results) const {
    if (!node || results.size() >= static_cast<size_t>(max_results)) {
        return;
    }
    
    // If this node marks end of word, add it to results
    if (node->is_end_of_word) {
        results.emplace_back(node->word, node->frequency);
    }
    
    // Recursively collect from all children
    for (auto& [c, child] : node->children) {
        if (results.size() >= static_cast<size_t>(max_results)) {
            break;
        }
        collect_suggestions(child.get(), results, max_results);
    }
}

double AutoComplete::calculate_relevance(const std::string& word, 
                                        const std::string& prefix,
                                        uint32_t frequency) const {
    // Weighted scoring:
    // 1. Frequency score (normalized)
    // 2. Length similarity (prefer shorter completions)
    // 3. Exact prefix match bonus
    
    double freq_score = std::log(frequency + 1);
    
    double length_penalty = 1.0 / (1.0 + (word.length() - prefix.length()) * 0.1);
    
    double exact_match_bonus = (word.substr(0, prefix.length()) == prefix) ? 1.2 : 1.0;
    
    return freq_score * length_penalty * exact_match_bonus;
}

std::vector<Suggestion> AutoComplete::get_suggestions(const std::string& prefix) const {
    return get_suggestions(prefix, max_suggestions);
}

std::vector<Suggestion> AutoComplete::get_suggestions(const std::string& prefix, 
                                                     size_t limit) const {
    std::vector<Suggestion> suggestions;
    
    // Check minimum prefix length
    if (prefix.length() < min_prefix_length) {
        return suggestions;
    }
    
    // Find the node corresponding to the prefix
    TrieNode* prefix_node = find_prefix_node(prefix);
    
    if (!prefix_node) {
        return suggestions; // Prefix not found
    }
    
    std::string normalized_prefix = normalize_text(prefix);
    
    // If prefix itself is a word, include it
    if (prefix_node->is_end_of_word) {
        double relevance = calculate_relevance(prefix_node->word, 
                                              normalized_prefix,
                                              prefix_node->frequency);
        suggestions.emplace_back(prefix_node->word, prefix_node->frequency, relevance);
    }
    
    // Collect all words with this prefix (get more than limit for better sorting)
    collect_suggestions(prefix_node, suggestions, limit * 3);
    
    // Calculate relevance scores
    for (auto& suggestion : suggestions) {
        suggestion.relevance_score = calculate_relevance(
            suggestion.word, 
            normalized_prefix,
            suggestion.frequency
        );
    }
    
    // Sort by relevance score (descending)
    std::sort(suggestions.begin(), suggestions.end(),
              [](const Suggestion& a, const Suggestion& b) {
                  if (std::abs(a.relevance_score - b.relevance_score) < 0.001) {
                      // If scores are very close, sort by frequency
                      if (a.frequency != b.frequency) {
                          return a.frequency > b.frequency;
                      }
                      // Then alphabetically
                      return a.word < b.word;
                  }
                  return a.relevance_score > b.relevance_score;
              });
    
    // Limit to requested size
    if (suggestions.size() > limit) 
        suggestions.erase(suggestions.begin() + limit, suggestions.end());
    

    
    return suggestions;
}

bool AutoComplete::word_exists(const std::string& word) const {
    if (word.empty()) return false;
    
    std::string normalized = normalize_text(word);
    TrieNode* current = root.get();
    
    for (char c : normalized) {
        if (current->children.find(c) == current->children.end()) {
            return false;
        }
        current = current->children[c].get();
    }
    
    return current->is_end_of_word;
}

bool AutoComplete::is_valid_prefix(const std::string& prefix) const {
    if (prefix.length() < min_prefix_length) {
        return false;
    }
    
    return find_prefix_node(prefix) != nullptr;
}

uint32_t AutoComplete::get_word_frequency(const std::string& word) const {
    if (word.empty()) return 0;
    
    std::string normalized = normalize_text(word);
    TrieNode* current = root.get();
    
    for (char c : normalized) {
        if (current->children.find(c) == current->children.end()) {
            return 0;
        }
        current = current->children[c].get();
    }
    
    return current->is_end_of_word ? current->frequency : 0;
}

void AutoComplete::clear() {
    root = std::make_unique<TrieNode>();
    total_words = 0;
}

void AutoComplete::print_statistics() const {
    std::cout << "\n=== AutoComplete Statistics ===" << std::endl;
    std::cout << "Total vocabulary: " << total_words << " words" << std::endl;
    std::cout << "Max suggestions: " << max_suggestions << std::endl;
    std::cout << "Min prefix length: " << min_prefix_length << std::endl;
    std::cout << "Case sensitive: " << (case_sensitive ? "Yes" : "No") << std::endl;
    std::cout << "Root children: " << root->children.size() << " branches" << std::endl;
    std::cout << "==============================\n" << std::endl;
}

std::unordered_map<std::string, std::vector<Suggestion>> 
AutoComplete::batch_suggestions(const std::vector<std::string>& prefixes) const {
    std::unordered_map<std::string, std::vector<Suggestion>> results;
    
    for (const auto& prefix : prefixes) {
        results[prefix] = get_suggestions(prefix);
    }
    
    return results;
}

// Simple Levenshtein distance for fuzzy matching
static int levenshtein_distance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));
    
    for (size_t i = 0; i <= len1; ++i) d[i][0] = i;
    for (size_t i = 0; i <= len2; ++i) d[0][i] = i;
    
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({
                d[i - 1][j] + 1,      // deletion
                d[i][j - 1] + 1,      // insertion
                d[i - 1][j - 1] + cost // substitution
            });
        }
    }
    
    return d[len1][len2];
}

std::vector<Suggestion> AutoComplete::fuzzy_search(const std::string& query, 
                                                  int max_edit_distance) const {
    std::vector<Suggestion> fuzzy_results;
    std::string normalized_query = normalize_text(query);
    
    // Simple BFS traversal to find similar words
    // Note: This is a simplified version. For production, consider using BK-tree
    std::function<void(TrieNode*, int)> traverse = [&](TrieNode* node, int depth) {
        if (!node) return;
        
        if (node->is_end_of_word) {
            int distance = levenshtein_distance(normalized_query, node->word);
            if (distance <= max_edit_distance) {
                double relevance = 1.0 / (1.0 + distance);
                fuzzy_results.emplace_back(node->word, node->frequency, relevance);
            }
        }
        
        for (auto& [c, child] : node->children) {
            traverse(child.get(), depth + 1);
        }
    };
    
    traverse(root.get(), 0);
    
    // Sort by relevance
    std::sort(fuzzy_results.begin(), fuzzy_results.end(),
              [](const Suggestion& a, const Suggestion& b) {
                  return a.relevance_score > b.relevance_score;
              });
    
    // Limit results
    if (fuzzy_results.size() > max_suggestions) {
        fuzzy_results.erase(
        fuzzy_results.begin() + max_suggestions,
        fuzzy_results.end()
    );
}

    
    return fuzzy_results;
}


bool AutoComplete::save_to_binary(const std::string& file_path) const {
    std::ofstream out(file_path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for writing" << std::endl;
        return false;
    }
    
    std::cout << "Saving AutoComplete to binary: " << file_path << std::endl;
    
    // Write header
    out.write(reinterpret_cast<const char*>(&total_words), sizeof(total_words));
    out.write(reinterpret_cast<const char*>(&max_suggestions), sizeof(max_suggestions));
    out.write(reinterpret_cast<const char*>(&min_prefix_length), sizeof(min_prefix_length));
    out.write(reinterpret_cast<const char*>(&case_sensitive), sizeof(case_sensitive));
    
    // Serialize trie using BFS
    std::queue<TrieNode*> q;
    q.push(root.get());
    
    uint32_t node_count = 0;
    
    // First pass: count nodes
    std::function<void(TrieNode*)> count_nodes = [&](TrieNode* node) {
        if (!node) return;
        node_count++;
        for (auto& [c, child] : node->children) {
            count_nodes(child.get());
        }
    };
    count_nodes(root.get());
    
    out.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    
    // Second pass: serialize nodes
    std::function<void(TrieNode*)> serialize = [&](TrieNode* node) {
        if (!node) return;
        
        // Write node data
        out.write(reinterpret_cast<const char*>(&node->is_end_of_word), sizeof(node->is_end_of_word));
        out.write(reinterpret_cast<const char*>(&node->frequency), sizeof(node->frequency));
        
        uint32_t word_len = node->word.length();
        out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));
        if (word_len > 0) {
            out.write(node->word.c_str(), word_len);
        }
        
        // Write children count
        uint32_t child_count = node->children.size();
        out.write(reinterpret_cast<const char*>(&child_count), sizeof(child_count));
        
        // Write each child
        for (auto& [c, child] : node->children) {
            out.write(reinterpret_cast<const char*>(&c), sizeof(c));
            serialize(child.get());
        }
    };
    
    serialize(root.get());
    out.close();
    
    std::cout << "✓ Saved " << total_words << " words (" << node_count << " nodes)" << std::endl;
    return true;
}

bool AutoComplete::load_from_binary(const std::string& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for reading" << std::endl;
        return false;
    }
    
    std::cout << "Loading AutoComplete from binary: " << file_path << std::endl;
    
    clear();
    
    // Read header
    in.read(reinterpret_cast<char*>(&total_words), sizeof(total_words));
    in.read(reinterpret_cast<char*>(&max_suggestions), sizeof(max_suggestions));
    in.read(reinterpret_cast<char*>(&min_prefix_length), sizeof(min_prefix_length));
    in.read(reinterpret_cast<char*>(&case_sensitive), sizeof(case_sensitive));
    
    uint32_t node_count;
    in.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    
    // Deserialize trie
    std::function<void(TrieNode*)> deserialize = [&](TrieNode* node) {
        if (!node) return;
        
        // Read node data
        in.read(reinterpret_cast<char*>(&node->is_end_of_word), sizeof(node->is_end_of_word));
        in.read(reinterpret_cast<char*>(&node->frequency), sizeof(node->frequency));
        
        uint32_t word_len;
        in.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));
        if (word_len > 0) {
            node->word.resize(word_len);
            in.read(&node->word[0], word_len);
        }
        
        // Read children
        uint32_t child_count;
        in.read(reinterpret_cast<char*>(&child_count), sizeof(child_count));
        
        for (uint32_t i = 0; i < child_count; i++) {
            char c;
            in.read(reinterpret_cast<char*>(&c), sizeof(c));
            
            node->children[c] = std::make_unique<TrieNode>();
            deserialize(node->children[c].get());
        }
    };
    
    deserialize(root.get());
    in.close();
    
    std::cout << "✓ Loaded " << total_words << " words (" << node_count << " nodes)" << std::endl;
    return true;
}
