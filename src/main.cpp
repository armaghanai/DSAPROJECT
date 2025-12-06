#include "../include/metadataparser.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

int main() {
    // =================== Configuration ===================
    std::string indices_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\indices\\sampleOutput\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";

    // =================== Load Lexicon ===================
    std::cout << "=== Loading Lexicon ===" << std::endl;
    LexiconBuilder lexicon;
    lexicon.load_from_csv(indices_path + "lexicon.csv");
    std::cout << "Lexicon loaded: " << lexicon.get_size() << " unique words" << std::endl;

    // =================== Load Inverted Index ===================
    std::cout << "\n=== Loading Inverted Index ===" << std::endl;
    std::unordered_map<uint32_t, std::string> reverse_lex = lexicon.build_reverse_lexicon();
    InvertedIndex inverted_index;
    inverted_index.load_from_binary(indices_path + "inverted_index.bin", reverse_lex);
    std::cout << "Inverted Index loaded" << std::endl;
    inverted_index.print_statistics();

    // =================== Create Barrels ===================
    std::cout << "\n=== Creating Barrels ===" << std::endl;
    inverted_index.create_barrels(barrel_path, reverse_lex, 4);
    inverted_index.print_barrel_info();

    // =================== Test Barrel Queries ===================
    std::cout << "\n=== Testing Barrel Queries ===" << std::endl;
    
    InvertedIndex query_idx;
    query_idx.load_barrel_metadata(barrel_path);
    query_idx.print_barrel_info();

    // Show first 20 words from lexicon to see what format they're in
    std::cout << "\n=== First 20 Words in Lexicon (checking format) ===" << std::endl;
    int count = 0;
    for (const auto& [word_id, word] : reverse_lex) {
        std::cout << "ID " << word_id << ": [" << word << "] (length: " << word.length() << ")" << std::endl;
        if (++count >= 20) break;
    }
    
    // Test different variations to find the right format
    std::cout << "\n=== Testing Different Word Formats ===" << std::endl;
    std::vector<std::string> test_variations = {
        "virus",
        "\"virus\"",
        "infection",
        "\"infection\"",
        "cells",
        "\"cells\""
    };
    
    for (const auto& word : test_variations) {
        uint32_t word_id = lexicon.get_word_id(word);
        if (word_id != UINT32_MAX) {
            std::cout << "✓ FOUND: [" << word << "] -> ID: " << word_id << std::endl;
        } else {
            std::cout << "✗ NOT FOUND: [" << word << "]" << std::endl;
        }
    }
    
    // Use words that we know exist (from CSV sample)
    std::vector<std::string> actual_words = {"virus", "infection", "cells", "protein", "patients"};
    
    // Try with quotes if needed
    std::vector<std::string> words_to_test;
    for (const auto& w : actual_words) {
        uint32_t wid = lexicon.get_word_id(w);
        if (wid == UINT32_MAX) {
            // Try with quotes
            std::string with_quotes = "\"" + w + "\"";
            wid = lexicon.get_word_id(with_quotes);
            if (wid != UINT32_MAX) {
                words_to_test.push_back(with_quotes);
            }
        } else {
            words_to_test.push_back(w);
        }
    }
    
    std::cout << "\n=== Testing Barrel Queries ===" << std::endl;
    std::cout << "Using " << words_to_test.size() << " words that exist in lexicon" << std::endl;
    
    for (const auto& word : words_to_test) {
        uint32_t word_id = lexicon.get_word_id(word);
        
        if (word_id == UINT32_MAX) {
            continue;
        }
        
        std::cout << "\n--- Searching: [" << word << "] (ID: " << word_id << ") ---" << std::endl;
        
        query_idx.load_barrel_for_word(word_id, reverse_lex);
        
        const auto* postings = query_idx.get_terms(word_id);
        
        if (postings) {
            std::cout << "Found in " << postings->size() << " documents" << std::endl;
            
            size_t cnt = 0;
            for (const auto& [doc_id, freq] : *postings) {
                std::cout << "  Doc " << doc_id << ": " << freq << " times" << std::endl;
                if (++cnt >= 5) break;
            }
        }
    }

    std::cout << "\n=== Done ===" << std::endl;
    return 0;
}