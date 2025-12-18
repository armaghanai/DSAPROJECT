
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <libstemmer.h>

using u32 = uint32_t;

class Lemmatizer {
private:
    sb_stemmer* stemmer;
    std::unordered_set<std::string> medical_preserve;
    std::unordered_set<std::string> medical_abbrev;
    std::unordered_set<std::string> noise_words;
    std::unordered_set<std::string> stop_words;
    std::unordered_set<std::string> common_words_dict;
    std::unordered_map<std::string, std::string> stemming_corrections;
    
    // Initialization methods
    void init_medical_terms();
    void init_stop_words();
    void init_common_words();
    void init_stemming_corrections();
    
    // Helper methods
    bool is_likely_noise(const std::string& text) const;
    bool is_stop_word(const std::string& word) const;
    std::string clean_token(const std::string& token);
    bool should_preserve(const std::string& word) const;
    std::string stem_word(const std::string& word);
    std::string correct_stemming(const std::string& stemmed);
    std::string improved_stem(const std::string& word);

public:
    // Constructor and Destructor
    Lemmatizer();
    ~Lemmatizer();
    
    // Main processing function - processes text and returns term frequencies
    void process_text(const std::string& text, 
                     std::unordered_map<std::string, u32>& term_frequencies);
    
    // Get unique lemmas from text (useful for lexicon building)
    std::unordered_set<std::string> get_unique_lemmas(const std::string& text);
    
    // Single word stemming
    std::string lemmatize_word(const std::string& word);
};

