#pragma once
#include <string>
#include <vector>
#include <set>
#include <unordered_set>

class TextPreprocessor {
public:
    TextPreprocessor();
    
    // Main preprocessing pipeline
    std::vector<std::string> preprocess(const std::string& text);
    
    // Individual processing steps
    std::string toLowerCase(const std::string& text);
    std::string removeSpecialChars(const std::string& text);
    std::vector<std::string> tokenize(const std::string& text);
    std::vector<std::string> removeStopWords(const std::vector<std::string>& tokens);
    std::string stemWord(const std::string& word);
    
    // Configuration setters
    void setRemoveStopWords(bool remove) { remove_stop_words = remove; }
    void setUseStemming(bool use) { use_stemming = use; }
    void setMinWordLength(int length) { min_word_length = length; }
    void setRemoveNumbers(bool remove) { remove_numbers = remove; }
    
    // Getters
    bool isStopWord(const std::string& word) const;
    
private:
    bool remove_stop_words;
    bool use_stemming;
    bool remove_numbers;
    int min_word_length;
    std::unordered_set<std::string> stop_words;
    
    void initializeStopWords();
    bool isValidWord(const std::string& word) const;
    bool isNumber(const std::string& word) const;
    
    // Porter Stemmer helper functions
    bool isConsonant(const std::string& word, int i) const;
    int measureWord(const std::string& word) const;
    bool containsVowel(const std::string& word) const;
    bool endsWithDoubleConsonant(const std::string& word) const;
    bool endsWithCVC(const std::string& word) const;
    std::string step1a(std::string word) const;
    std::string step1b(std::string word) const;
    std::string step1c(std::string word) const;
    std::string step2(std::string word) const;
    std::string step3(std::string word) const;
    std::string step4(std::string word) const;
    std::string step5a(std::string word) const;
    std::string step5b(std::string word) const;
};