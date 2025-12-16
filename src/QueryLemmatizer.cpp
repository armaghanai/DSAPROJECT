#include "../include/QueryLemmatizer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

QueryLemmatizer::QueryLemmatizer() {
    initializeCommonLemmas();
}

void QueryLemmatizer::initializeCommonLemmas() {
    // Common English lemmatization rules
    word_forms["running"] = "run";
    word_forms["runs"] = "run";
    word_forms["ran"] = "run";
    word_forms["running"] = "run";
    
    word_forms["studies"] = "study";
    word_forms["studied"] = "study";
    word_forms["studying"] = "study";
    
    word_forms["research"] = "research";
    word_forms["researching"] = "research";
    word_forms["researched"] = "research";
    
    word_forms["analysis"] = "analysis";
    word_forms["analyses"] = "analysis";
    word_forms["analyze"] = "analysis";
    word_forms["analyzed"] = "analysis";
    word_forms["analyzing"] = "analysis";
    
    word_forms["data"] = "data";
    word_forms["datum"] = "data";
    
    word_forms["finding"] = "find";
    word_forms["findings"] = "find";
    word_forms["found"] = "find";
    
    word_forms["virus"] = "virus";
    word_forms["viruses"] = "virus";
    word_forms["viral"] = "virus";
    
    word_forms["disease"] = "disease";
    word_forms["diseases"] = "disease";
    
    word_forms["treatment"] = "treatment";
    word_forms["treatments"] = "treatment";
    word_forms["treat"] = "treatment";
    word_forms["treating"] = "treatment";
    word_forms["treated"] = "treatment";
    
    word_forms["vaccine"] = "vaccine";
    word_forms["vaccines"] = "vaccine";
    word_forms["vaccination"] = "vaccine";
    word_forms["vaccinated"] = "vaccine";
    word_forms["vaccinating"] = "vaccine";
}

std::string QueryLemmatizer::lemmatize(const std::string& word) {
    // Check cache first
    auto cache_it = lemma_cache.find(word);
    if (cache_it != lemma_cache.end()) {
        return cache_it->second;
    }
    
    // Check predefined word forms
    auto form_it = word_forms.find(word);
    if (form_it != word_forms.end()) {
        lemma_cache[word] = form_it->second;
        return form_it->second;
    }
    
    // Apply lemmatization rules
    std::string lemma = applyLemmaRules(word);
    lemma_cache[word] = lemma;
    return lemma;
}

std::string QueryLemmatizer::applyLemmaRules(const std::string& word) {
    if (word.length() < 4) return word;
    
    std::string result = word;
    
    // Remove common suffixes
    if (result.length() > 3 && result.substr(result.length() - 3) == "ing") {
        return result.substr(0, result.length() - 3);
    }
    if (result.length() > 3 && result.substr(result.length() - 3) == "ies") {
        return result.substr(0, result.length() - 3) + "y";
    }
    if (result.length() > 2 && result.substr(result.length() - 2) == "es") {
        return result.substr(0, result.length() - 2);
    }
    if (result.length() > 1 && result[result.length() - 1] == 's') {
        return result.substr(0, result.length() - 1);
    }
    if (result.length() > 3 && result.substr(result.length() - 3) == "ous") {
        return result.substr(0, result.length() - 3);
    }
    if (result.length() > 3 && result.substr(result.length() - 3) == "ial") {
        return result.substr(0, result.length() - 3);
    }
    if (result.length() > 4 && result.substr(result.length() - 4) == "tion") {
        return result.substr(0, result.length() - 4);
    }
    if (result.length() > 4 && result.substr(result.length() - 4) == "ment") {
        return result.substr(0, result.length() - 4);
    }
    
    return result;
}

std::vector<std::string> QueryLemmatizer::lemmatize_tokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> lemmatized;
    lemmatized.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        lemmatized.push_back(lemmatize(token));
    }
    
    return lemmatized;
}

void QueryLemmatizer::add_lemma_rule(const std::string& word, const std::string& lemma) {
    word_forms[word] = lemma;
    lemma_cache.clear(); // Clear cache when adding new rules
}

bool QueryLemmatizer::load_lemmas_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open lemma file: " << file_path << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string word, lemma;
        if (iss >> word >> lemma) {
            word_forms[word] = lemma;
        }
    }
    
    file.close();
    std::cout << "Loaded " << word_forms.size() << " lemma rules" << std::endl;
    return true;
}

bool QueryLemmatizer::save_lemmas_to_file(const std::string& file_path) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << file_path << std::endl;
        return false;
    }
    
    file << "# Word -> Lemma mappings\n";
    for (const auto& [word, lemma] : word_forms) {
        file << word << " " << lemma << "\n";
    }
    
    file.close();
    return true;
}