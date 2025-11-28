#include "textpreprocessor.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

TextPreprocessor::TextPreprocessor() 
    : remove_stop_words(true), 
      use_stemming(false),
      remove_numbers(true),
      min_word_length(2) {
    initializeStopWords();
}

void TextPreprocessor::initializeStopWords() {
    // Common English stop words
    std::vector<std::string> words = {
        "a", "about", "above", "after", "again", "against", "all", "am", "an", 
        "and", "any", "are", "aren't", "as", "at", "be", "because", "been", 
        "before", "being", "below", "between", "both", "but", "by", "can't", 
        "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't", 
        "doing", "don't", "down", "during", "each", "few", "for", "from", 
        "further", "had", "hadn't", "has", "hasn't", "have", "haven't", "having", 
        "he", "he'd", "he'll", "he's", "her", "here", "here's", "hers", "herself", 
        "him", "himself", "his", "how", "how's", "i", "i'd", "i'll", "i'm", 
        "i've", "if", "in", "into", "is", "isn't", "it", "it's", "its", "itself", 
        "let's", "me", "more", "most", "mustn't", "my", "myself", "no", "nor", 
        "not", "of", "off", "on", "once", "only", "or", "other", "ought", "our", 
        "ours", "ourselves", "out", "over", "own", "same", "shan't", "she", 
        "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such", 
        "than", "that", "that's", "the", "their", "theirs", "them", "themselves", 
        "then", "there", "there's", "these", "they", "they'd", "they'll", 
        "they're", "they've", "this", "those", "through", "to", "too", "under", 
        "until", "up", "very", "was", "wasn't", "we", "we'd", "we'll", "we're", 
        "we've", "were", "weren't", "what", "what's", "when", "when's", "where", 
        "where's", "which", "while", "who", "who's", "whom", "why", "why's", 
        "with", "won't", "would", "wouldn't", "you", "you'd", "you'll", "you're", 
        "you've", "your", "yours", "yourself", "yourselves"
    };
    
    for (const auto& word : words) {
        stop_words.insert(word);
    }
}

std::vector<std::string> TextPreprocessor::preprocess(const std::string& text) {
    // Step 1: Convert to lowercase
    std::string lower_text = toLowerCase(text);
    
    // Step 2: Remove special characters
    std::string clean_text = removeSpecialChars(lower_text);
    
    // Step 3: Tokenize
    std::vector<std::string> tokens = tokenize(clean_text);
    
    // Step 4: Remove stop words (if enabled)
    if (remove_stop_words) {
        tokens = removeStopWords(tokens);
    }
    
    // Step 5: Apply stemming (if enabled)
    if (use_stemming) {
        for (auto& token : tokens) {
            token = stemWord(token);
        }
    }
    
    // Step 6: Filter by length and validity
    std::vector<std::string> filtered_tokens;
    for (const auto& token : tokens) {
        if (isValidWord(token)) {
            filtered_tokens.push_back(token);
        }
    }
    
    return filtered_tokens;
}

std::string TextPreprocessor::toLowerCase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string TextPreprocessor::removeSpecialChars(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c))) {
            result += c;
        } else {
            // Replace special chars with space to separate words
            if (!result.empty() && result.back() != ' ') {
                result += ' ';
            }
        }
    }
    
    return result;
}

std::vector<std::string> TextPreprocessor::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::vector<std::string> TextPreprocessor::removeStopWords(const std::vector<std::string>& tokens) {
    std::vector<std::string> filtered;
    
    for (const auto& token : tokens) {
        if (!isStopWord(token)) {
            filtered.push_back(token);
        }
    }
    
    return filtered;
}

bool TextPreprocessor::isStopWord(const std::string& word) const {
    return stop_words.find(word) != stop_words.end();
}

bool TextPreprocessor::isValidWord(const std::string& word) const {
    // Check minimum length
    if (word.length() < static_cast<size_t>(min_word_length)) {
        return false;
    }
    
    // Check if it's a number (if we're removing numbers)
    if (remove_numbers && isNumber(word)) {
        return false;
    }
    
    // Check if word contains at least one letter
    bool has_letter = false;
    for (char c : word) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            has_letter = true;
            break;
        }
    }
    
    return has_letter;
}

bool TextPreprocessor::isNumber(const std::string& word) const {
    if (word.empty()) return false;
    
    for (char c : word) {
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.' && c != ',') {
            return false;
        }
    }
    return true;
}

// ==================== PORTER STEMMER IMPLEMENTATION ====================

std::string TextPreprocessor::stemWord(const std::string& word) {
    if (word.length() <= 2) return word;
    
    std::string result = word;
    result = step1a(result);
    result = step1b(result);
    result = step1c(result);
    result = step2(result);
    result = step3(result);
    result = step4(result);
    result = step5a(result);
    result = step5b(result);
    
    return result;
}

bool TextPreprocessor::isConsonant(const std::string& word, int i) const {
    char c = word[i];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
        return false;
    }
    if (c == 'y') {
        return (i == 0) ? true : !isConsonant(word, i - 1);
    }
    return true;
}

int TextPreprocessor::measureWord(const std::string& word) const {
    int measure = 0;
    int i = 0;
    int length = word.length();
    
    // Skip initial consonants
    while (i < length && isConsonant(word, i)) {
        i++;
    }
    
    // Count VC sequences
    while (i < length) {
        // Skip vowels
        while (i < length && !isConsonant(word, i)) {
            i++;
        }
        if (i >= length) break;
        
        // Skip consonants
        while (i < length && isConsonant(word, i)) {
            i++;
        }
        measure++;
    }
    
    return measure;
}

bool TextPreprocessor::containsVowel(const std::string& word) const {
    for (size_t i = 0; i < word.length(); i++) {
        if (!isConsonant(word, i)) {
            return true;
        }
    }
    return false;
}

bool TextPreprocessor::endsWithDoubleConsonant(const std::string& word) const {
    int length = word.length();
    if (length < 2) return false;
    
    return (word[length - 1] == word[length - 2]) && 
           isConsonant(word, length - 1);
}

bool TextPreprocessor::endsWithCVC(const std::string& word) const {
    int length = word.length();
    if (length < 3) return false;
    
    char last = word[length - 1];
    if (last == 'w' || last == 'x' || last == 'y') return false;
    
    return isConsonant(word, length - 1) && 
           !isConsonant(word, length - 2) && 
           isConsonant(word, length - 3);
}

std::string TextPreprocessor::step1a(std::string word) const {
    // SSES -> SS
    if (word.length() >= 4 && word.substr(word.length() - 4) == "sses") {
        return word.substr(0, word.length() - 2);
    }
    // IES -> I
    if (word.length() >= 3 && word.substr(word.length() - 3) == "ies") {
        return word.substr(0, word.length() - 2);
    }
    // SS -> SS
    if (word.length() >= 2 && word.substr(word.length() - 2) == "ss") {
        return word;
    }
    // S -> (empty)
    if (word.length() >= 1 && word[word.length() - 1] == 's') {
        return word.substr(0, word.length() - 1);
    }
    return word;
}

std::string TextPreprocessor::step1b(std::string word) const {
    bool second_or_third = false;
    
    // (m>0) EED -> EE
    if (word.length() >= 3 && word.substr(word.length() - 3) == "eed") {
        std::string stem = word.substr(0, word.length() - 3);
        if (measureWord(stem) > 0) {
            return stem + "ee";
        }
        return word;
    }
    
    // (*v*) ED -> (empty)
    if (word.length() >= 2 && word.substr(word.length() - 2) == "ed") {
        std::string stem = word.substr(0, word.length() - 2);
        if (containsVowel(stem)) {
            word = stem;
            second_or_third = true;
        }
    }
    
    // (*v*) ING -> (empty)
    if (!second_or_third && word.length() >= 3 && word.substr(word.length() - 3) == "ing") {
        std::string stem = word.substr(0, word.length() - 3);
        if (containsVowel(stem)) {
            word = stem;
            second_or_third = true;
        }
    }
    
    if (second_or_third) {
        // AT -> ATE
        if (word.length() >= 2 && word.substr(word.length() - 2) == "at") {
            return word + "e";
        }
        // BL -> BLE
        if (word.length() >= 2 && word.substr(word.length() - 2) == "bl") {
            return word + "e";
        }
        // IZ -> IZE
        if (word.length() >= 2 && word.substr(word.length() - 2) == "iz") {
            return word + "e";
        }
        // (*d and not (*L or *S or *Z)) -> single letter
        if (endsWithDoubleConsonant(word)) {
            char last = word[word.length() - 1];
            if (last != 'l' && last != 's' && last != 'z') {
                return word.substr(0, word.length() - 1);
            }
        }
        // (m=1 and *o) -> E
        if (measureWord(word) == 1 && endsWithCVC(word)) {
            return word + "e";
        }
    }
    
    return word;
}

std::string TextPreprocessor::step1c(std::string word) const {
    // (*v*) Y -> I
    if (word.length() >= 1 && word[word.length() - 1] == 'y') {
        std::string stem = word.substr(0, word.length() - 1);
        if (containsVowel(stem)) {
            return stem + "i";
        }
    }
    return word;
}

std::string TextPreprocessor::step2(std::string word) const {
    if (word.length() < 2) return word;
    
    char penultimate = word[word.length() - 2];
    
    switch (penultimate) {
        case 'a':
            if (word.length() >= 7 && word.substr(word.length() - 7) == "ational") {
                std::string stem = word.substr(0, word.length() - 7);
                if (measureWord(stem) > 0) return stem + "ate";
            }
            if (word.length() >= 6 && word.substr(word.length() - 6) == "tional") {
                std::string stem = word.substr(0, word.length() - 6);
                if (measureWord(stem) > 0) return stem + "tion";
            }
            break;
        case 'c':
            if (word.length() >= 4 && word.substr(word.length() - 4) == "enci") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem + "ence";
            }
            if (word.length() >= 4 && word.substr(word.length() - 4) == "anci") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem + "ance";
            }
            break;
        case 'e':
            if (word.length() >= 4 && word.substr(word.length() - 4) == "izer") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem + "ize";
            }
            break;
        case 'l':
            if (word.length() >= 3 && word.substr(word.length() - 3) == "bli") {
                std::string stem = word.substr(0, word.length() - 3);
                if (measureWord(stem) > 0) return stem + "ble";
            }
            if (word.length() >= 4 && word.substr(word.length() - 4) == "alli") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem + "al";
            }
            if (word.length() >= 5 && word.substr(word.length() - 5) == "entli") {
                std::string stem = word.substr(0, word.length() - 5);
                if (measureWord(stem) > 0) return stem + "ent";
            }
            break;
    }
    
    return word;
}

std::string TextPreprocessor::step3(std::string word) const {
    if (word.length() < 3) return word;
    
    char last = word[word.length() - 1];
    
    switch (last) {
        case 'e':
            if (word.length() >= 5 && word.substr(word.length() - 5) == "icate") {
                std::string stem = word.substr(0, word.length() - 5);
                if (measureWord(stem) > 0) return stem + "ic";
            }
            if (word.length() >= 5 && word.substr(word.length() - 5) == "ative") {
                std::string stem = word.substr(0, word.length() - 5);
                if (measureWord(stem) > 0) return stem;
            }
            if (word.length() >= 5 && word.substr(word.length() - 5) == "alize") {
                std::string stem = word.substr(0, word.length() - 5);
                if (measureWord(stem) > 0) return stem + "al";
            }
            break;
        case 'i':
            if (word.length() >= 5 && word.substr(word.length() - 5) == "iciti") {
                std::string stem = word.substr(0, word.length() - 5);
                if (measureWord(stem) > 0) return stem + "ic";
            }
            break;
        case 'l':
            if (word.length() >= 4 && word.substr(word.length() - 4) == "ical") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem + "ic";
            }
            if (word.length() >= 3 && word.substr(word.length() - 3) == "ful") {
                std::string stem = word.substr(0, word.length() - 3);
                if (measureWord(stem) > 0) return stem;
            }
            break;
        case 's':
            if (word.length() >= 4 && word.substr(word.length() - 4) == "ness") {
                std::string stem = word.substr(0, word.length() - 4);
                if (measureWord(stem) > 0) return stem;
            }
            break;
    }
    
    return word;
}

std::string TextPreprocessor::step4(std::string word) const {
    if (word.length() < 2) return word;
    
    char penultimate = word[word.length() - 2];
    
    std::vector<std::string> suffixes = {
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant", 
        "ement", "ment", "ent", "ion", "ou", "ism", "ate", "iti", "ous", "ive", "ize"
    };
    
    for (const auto& suffix : suffixes) {
        if (word.length() >= suffix.length() && 
            word.substr(word.length() - suffix.length()) == suffix) {
            std::string stem = word.substr(0, word.length() - suffix.length());
            
            // Special case for "ion"
            if (suffix == "ion" && stem.length() > 0) {
                char last_stem_char = stem[stem.length() - 1];
                if (last_stem_char != 's' && last_stem_char != 't') {
                    continue;
                }
            }
            
            if (measureWord(stem) > 1) {
                return stem;
            }
        }
    }
    
    return word;
}

std::string TextPreprocessor::step5a(std::string word) const {
    if (word.length() >= 1 && word[word.length() - 1] == 'e') {
        std::string stem = word.substr(0, word.length() - 1);
        int measure = measureWord(stem);
        
        if (measure > 1) {
            return stem;
        }
        if (measure == 1 && !endsWithCVC(stem)) {
            return stem;
        }
    }
    
    return word;
}

std::string TextPreprocessor::step5b(std::string word) const {
    if (measureWord(word) > 1 && 
        endsWithDoubleConsonant(word) && 
        word[word.length() - 1] == 'l') {
        return word.substr(0, word.length() - 1);
    }
    
    return word;
}