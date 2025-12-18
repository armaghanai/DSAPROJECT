#include "../include/Lemmatizer.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <regex>

Lemmatizer::Lemmatizer() : stemmer(nullptr) 
{
    stemmer = sb_stemmer_new("english", "UTF_8");
    if (stemmer == nullptr) {
        std::cerr << "Failed to create stemmer for english\n";
    }
    
    init_medical_terms();
    init_stop_words();
    init_common_words();
    init_stemming_corrections();
    
    std::cout << "Enhanced Lemmatizer initialized\n";
}

Lemmatizer::~Lemmatizer() {
    if (stemmer != nullptr) {
        sb_stemmer_delete(stemmer);
    }
}

void Lemmatizer::init_medical_terms() 
{
    medical_preserve = {
        "covid", "covid-19", "covid19", "sars", "sars-cov", "sars-cov-2",
        "mers", "mers-cov", "coronavirus", "coronaviruses",
        "antibody", "antibodies", "antigen", "antigens", "vaccine", "vaccines", "vaccination",
        "virus", "viruses", "viral", "virion", "virions", "protein", "proteins", "peptide", "peptides",
        "rna", "dna", "mrna", "trna", "rrna", "genome", "genomic", "gene", "genes",
        "cell", "cells", "cellular", "cytokine", "cytokines", "pneumonia", "influenza", "respiratory",
        "pulmonary", "syndrome", "disease", "infection", "infectious", "transmission", "transmissible",
        "contagious", "symptom", "symptoms", "asymptomatic", "symptomatic", "diagnosis", "diagnostic",
        "prognosis", "prognostic", "treatment", "therapeutic", "therapy", "therapies", "immune", "immunity",
        "immunology", "immunological", "pathogen", "pathogens", "pathogenic", "pathogenesis", "pandemic",
        "epidemic", "endemic", "outbreak", "mortality", "morbidity", "fatality", "patient", "patients",
        "clinical", "hospital", "icu", "intensive", "ventilator", "ventilation", "oxygen", "hypoxia",
        "hypoxic", "inflammation", "inflammatory", "receptor", "receptors", "ace2", "spike", "nucleocapsid",
        "membrane", "antibacterial", "antiviral", "antimicrobial"
    };
    
    medical_abbrev = {
        "who", "cdc", "fda", "nih", "niaid", "ema", "pcr", "rt-pcr", "qrt-pcr", "elisa",
        "ards", "icu", "ecmo", "il", "il-6", "il-1", "tnf", "tnf-alpha", "ifn", "igg", "igm",
        "iga", "ige", "hiv", "aids", "tb", "ebv", "cmv", "ct", "mri", "xray", "ecg", "ekg",
        "mg", "ml", "kg", "mcg", "ng", "pg", "µg", "usa", "uk", "eu"
    };
    
    noise_words = {
        "vs", "ad", "ed", "ll", "b", "n", "a", "r", "see", "october", "bs", "epal", "z"
    };
}

void Lemmatizer::init_stop_words() 
{
    stop_words = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
        "of", "with", "by", "about", "against", "between", "into", "through",
        "during", "before", "after", "above", "below", "from", "up", "down",
        "out", "off", "over", "under", "again", "further", "then", "once",
        "here", "there", "when", "where", "why", "how", "all", "any", "both",
        "each", "few", "more", "most", "other", "some", "such", "no", "nor",
        "not", "only", "own", "same", "so", "than", "too", "very", "s", "t",
        "can", "will", "just", "don", "should", "now", "is", "are", "was",
        "were", "be", "been", "being", "have", "has", "had", "having", "do",
        "does", "did", "doing", "would", "could", "should", "may", "might",
        "must", "shall", "will", "that", "use", "this", "these", "which", "also",
        "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"
    };
}

void Lemmatizer::init_common_words() 
{
    common_words_dict = {
        // Common nouns
        "sequence", "analysis", "structure", "figure", "patient", "model",
        "protein", "system", "region", "domain", "cell", "gene", "virus",
        "vaccine", "antibody", "treatment", "therapy", "hospital", "study",
        "research", "science", "medical", "clinical", "health", "disease",
        "infection", "symptom", "transmission", "mortality", "morbidity",
        "diagnosis", "prognosis", "treatment", "prevention", "control",
        
        // Common adjectives
        "different", "various", "several", "viral", "bacterial", "medical",
        "clinical", "scientific", "important", "significant", "critical",
        "severe", "mild", "acute", "chronic", "positive", "negative",
        
        // Common verbs
        "develop", "analyze", "study", "research", "test", "measure",
        "compare", "evaluate", "assess", "determine", "identify", "detect",
        "treat", "prevent", "control", "manage", "monitor", "observe"
    };
}

void Lemmatizer::init_stemming_corrections() 
{
    stemming_corrections = {
        {"sequenc", "sequence"},
        {"analysi", "analysis"},
        {"structur", "structure"},
        {"figur", "figure"},
        {"howev", "however"},
        {"differ", "different"},
        {"inform", "information"},
        {"activ", "active"},
        {"product", "produce"},
        {"detect", "detection"},
        {"infect", "infection"},
        {"develop", "development"},
        {"measur", "measure"},
        {"test", "testing"},
        {"treat", "treatment"},
        {"prevent", "prevention"},
        {"control", "control"},
        {"manag", "manage"},
        {"monitor", "monitoring"},
        {"observ", "observe"},
        
        // Medical term specific corrections
        {"pneumonia", "pneumonia"},
        {"influenza", "influenza"},
        {"respiratori", "respiratory"},
        {"immun", "immune"},
        {"cytokin", "cytokine"},
        {"pathogen", "pathogen"},
        {"genom", "genome"},
        {"prote", "protein"},
        {"peptid", "peptide"},
        {"vaccin", "vaccine"},
        {"antibodi", "antibody"},
        {"antigen", "antigen"},
        {"receptor", "receptor"},
        {"membrane", "membrane"}
    };
}

bool Lemmatizer::is_likely_noise(const std::string& text) const 
{
    if (text.length() == 1 && text != "a" && text != "i" && 
        medical_abbrev.find(text) == medical_abbrev.end()) {
        return true;
    }
    
    if (noise_words.find(text) != noise_words.end()) {
        return true;
    }
    
    if (text.empty()) return false;
    
    if (text[0] == '-' || text[0] == '.' || text[0] == ',' || 
        text[0] == '|' || text[0] == '/' || text[0] == '\\' ||
        text.back() == '-' || text.back() == '.' || text.back() == ',' ||
        text.back() == '|' || text.back() == '/' || text.back() == '\\') {
        return true;
    }
    
    std::regex url_pattern("https?://|www\\.|\\.(com|org|edu|gov|net|html?|php|asp|pdf)");
    if (std::regex_search(text, url_pattern)) {
        return true;
    }
    
    if (text.length() > 25) {
        return true;
    }
    
    int digit_count = std::count_if(text.begin(), text.end(), ::isdigit);
    return digit_count > text.length() * 0.3;
}

bool Lemmatizer::is_stop_word(const std::string& word) const {
    return stop_words.find(word) != stop_words.end();
}

std::string Lemmatizer::clean_token(const std::string& token) {
    std::string cleaned = token;
    
    // Remove punctuation from beginning and end
    while (!cleaned.empty() && ispunct(static_cast<unsigned char>(cleaned.front()))) {
        cleaned.erase(0, 1);
    }
    while (!cleaned.empty() && ispunct(static_cast<unsigned char>(cleaned.back()))) {
        cleaned.pop_back();
    }
    
    return cleaned;
}

bool Lemmatizer::should_preserve(const std::string& word) const {
    return medical_preserve.find(word) != medical_preserve.end() ||
           medical_abbrev.find(word) != medical_abbrev.end();
}

std::string Lemmatizer::stem_word(const std::string& word) {
    if ((stemmer == nullptr) || word.empty()) {
        return word;
    }
    
    const sb_symbol* stemmed = sb_stemmer_stem(
        stemmer, 
        reinterpret_cast<const sb_symbol*>(word.c_str()), 
        static_cast<int>(word.length())
    );
    
    return stemmed ? std::string(reinterpret_cast<const char*>(stemmed)) : word;
}

std::string Lemmatizer::correct_stemming(const std::string& stemmed) {
    auto it = stemming_corrections.find(stemmed);
    return (it != stemming_corrections.end()) ? it->second : stemmed;
}

std::string Lemmatizer::improved_stem(const std::string& word) {
    if (word.empty()) return word;
    
    // Check if we should preserve this word
    if (should_preserve(word)) {
        return word;
    }
    
    // Check if it's a common word we want to keep
    if (common_words_dict.find(word) != common_words_dict.end()) {
        // For common words, only remove simple plural 's'
        if (word.length() > 3 && word.back() == 's') {
            std::string singular = word.substr(0, word.length() - 1);
            if (common_words_dict.find(singular) != common_words_dict.end()) {
                return singular;
            }
        }
        return word;
    }
    
    // Apply Snowball stemming
    std::string stemmed = stem_word(word);
    
    // Apply corrections if needed
    auto it = stemming_corrections.find(stemmed);
    if (it != stemming_corrections.end()) {
        return it->second;
    }
    
    // Check if stemmed word looks reasonable
    if (stemmed.length() >= 3 && stemmed != word) {
        // Only accept stemming if it looks like a real word
        if (stemmed.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos) {
            return word; // Keep original if stemmed looks weird
        }
    }
    
    return stemmed;
}

void Lemmatizer::process_text(const std::string& text, 
                             std::unordered_map<std::string, u32>& term_frequencies) 
{
    term_frequencies.clear();
    
    if (text.empty()) { return; }
    
    // Clean text
    std::string cleaned_text = text;
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\n', ' ');
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\r', ' ');
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\t', ' ');
    
    // Remove control characters
    cleaned_text.erase(
        std::remove_if(cleaned_text.begin(), cleaned_text.end(),
            [](unsigned char c) {
                return c == '\0' || (c < 32 && c != ' ');
            }
        ),
        cleaned_text.end()
    );
    
    // Convert to lowercase
    std::transform(cleaned_text.begin(), cleaned_text.end(), cleaned_text.begin(), ::tolower);
    
    // Tokenize
    std::istringstream iss(cleaned_text);
    std::string token;
    
    while (iss >> token) {
        std::string cleaned = clean_token(token);
        
        if (cleaned.empty() || is_likely_noise(cleaned)) {
            continue;
        }
        
        // Skip very short words (unless medical abbreviation)
        if (cleaned.length() < 3 && medical_abbrev.find(cleaned) == medical_abbrev.end()) {
            continue;
        }
        
        // Remove stop words more aggressively
        if (is_stop_word(cleaned) && !should_preserve(cleaned)) {
            continue;
        }
        
        std::string final_term;
        
        // Apply appropriate processing
        if (should_preserve(cleaned)) {
            final_term = cleaned; // Keep medical terms as-is
        } else if (common_words_dict.find(cleaned) != common_words_dict.end()) {
            // For common words, use improved stemming
            final_term = improved_stem(cleaned);
        } else {
            // For other words, use standard stemming with correction
            std::string stemmed = stem_word(cleaned);
            final_term = correct_stemming(stemmed);
        }
        
        // Final validation
        if (final_term.empty() || final_term.length() < 2 || 
            is_likely_noise(final_term)) {
            continue;
        }
        
        // Add to frequencies
        term_frequencies[final_term]++;
    }
}

std::unordered_set<std::string> Lemmatizer::get_unique_lemmas(const std::string& text) {
    std::unordered_map<std::string, u32> term_freqs;
    process_text(text, term_freqs);
    
    std::unordered_set<std::string> unique_lemmas;
    for (const auto& [term, freq] : term_freqs) {
        unique_lemmas.insert(term);
    }
    
    return unique_lemmas;
}

std::string Lemmatizer::lemmatize_word(const std::string& word) {
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    std::string cleaned = clean_token(lower);
    if (cleaned.empty() || is_likely_noise(cleaned) || is_stop_word(cleaned)) {
        return "";
    }
    
    return improved_stem(cleaned);
}