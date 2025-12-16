
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class QueryLemmatizer {
private:
    std::unordered_map<std::string, std::string> lemma_cache;
    std::unordered_map<std::string, std::string> word_forms;
    
    // Common lemmatization rules
    void initializeCommonLemmas();
    std::string applyLemmaRules(const std::string& word);
    
public:
    QueryLemmatizer();
    
    // Lemmatize a single word
    std::string lemmatize(const std::string& word);
    
    // Lemmatize a list of tokens
    std::vector<std::string> lemmatize_tokens(const std::vector<std::string>& tokens);
    
    // Add custom lemma mappings
    void add_lemma_rule(const std::string& word, const std::string& lemma);
    
    // Load lemmas from file for faster performance
    bool load_lemmas_from_file(const std::string& file_path);
    bool save_lemmas_to_file(const std::string& file_path);
};