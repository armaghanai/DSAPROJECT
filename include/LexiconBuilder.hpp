#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

class LexiconBuilder
{
private:
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> lexicon_data;
    uint32_t next_word_id;  // Changed from int to uint32_t for consistency

public:
    LexiconBuilder();
    
    // Add and query words
    uint32_t add_word(const std::string& word, uint32_t count = 1);
    bool contains(const std::string& word);
    const std::pair<uint32_t, uint32_t>* get_word_details(const std::string& word);
    uint32_t get_frequency(const std::string& word) const;
    uint32_t get_word_id(const std::string& word) const;
    size_t get_size() const;
    
    // Save and load
    void save_to_csv(const std::string& csv_path);
    bool load_from_csv(const std::string& csv_path);
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> get_lexicon_data()const;
    
    // Reverse lexicon methods
    std::unordered_map<uint32_t, std::string> build_reverse_lexicon();  // Non-const (builds on call)
    std::unordered_map<uint32_t, std::string> get_reverse_lexicon() const;  // Const version (preferred)
    
    // Utility
    void clear_lexicon();
};