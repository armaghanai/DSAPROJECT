#include "../include/CachedSearchEngine.hpp"
#include "../include/ParallelSemanticSearch.hpp"
#include "../include/WordEmbeddingsEngine.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/AutoComplete.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>

// Timing utility
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    std::string label;
public:
    Timer(const std::string& label) : label(label) {
        std::cout << "[TIMING] Starting: " << label << "..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
    }
    
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "[TIMING] " << label << " completed in " 
                  << duration.count() << " ms" << std::endl << std::endl;
    }
};

int main() {
    // =================== Configuration ===================
    std::string indices_path = "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    std::string binary_embedding_file = 
    "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\embeddings\\glove.6B.100d.bin";
    std::string autocomplete_cache = indices_path + "autocomplete.bin";
    std::cout << "=== COVID-19 Research Paper Search Engine ===" << std::endl;
    std::cout << "With BM25 Keyword + Semantic Search + AutoComplete" << std::endl;
    std::cout << "==========================================\n" << std::endl;
    
    // =================== Step 1: Load Lexicon ===================
    {
        Timer t("Step 1: Loading Lexicon");
        LexiconBuilder lexicon;
        if (!lexicon.load_from_csv(indices_path + "lexicon.csv")) {
            std::cerr << "Error: Failed to load lexicon!" << std::endl;
            return 1;
        }
        std::cout << "Loaded " << lexicon.get_size() << " words" << std::endl;
    }
    
    // Reload lexicon for use
    LexiconBuilder lexicon;
    lexicon.load_from_csv(indices_path + "lexicon.csv");
    auto reverse_lex = lexicon.build_reverse_lexicon();
    
    // =================== Step 2: Load Forward Index ===================
    ForwardIndex forward_index;
    {
        Timer t("Step 2: Loading Forward Index");
        if (!forward_index.load_from_binary(indices_path + "forward_index.bin")) {
            std::cerr << "Error: Failed to load forward index!" << std::endl;
            return 1;
        }
        forward_index.print_statistics();
    }
    
    // =================== Step 3: Load Inverted Index ===================
    InvertedIndex inverted_index;
    {
        Timer t("Step 3: Loading Inverted Index");
        if (!inverted_index.load_from_binary(indices_path + "inverted_index.bin", reverse_lex)) {
            std::cerr << "Error: Failed to load inverted index!" << std::endl;
            return 1;
        }
        inverted_index.print_statistics();
    }
    
    // =================== Step 4: Setup Barrels ===================
    {
        Timer t("Step 4: Setting up Barrels");
        if (!std::filesystem::exists(barrel_path)) {
            std::cout << "Creating new barrels..." << std::endl;
            uint32_t num_barrels = 10;
            if (!inverted_index.create_barrels(barrel_path, reverse_lex, num_barrels)) {
                std::cerr << "Error: Failed to create barrels!" << std::endl;
                return 1;
            }
        }
        
        if (!inverted_index.load_barrel_metadata(barrel_path)) {
            std::cerr << "Error: Failed to load barrel metadata!" << std::endl;
            return 1;
        }
    }
    
    // =================== Step 5: Initialize BM25 Search Engine ===================
    TextPreprocessor preprocessor;
    CachedSearchEngine bm25_engine(&lexicon, &forward_index, &inverted_index, &preprocessor);
    
    // =================== Step 6: Load Word Embeddings ===================
    WordEmbeddingsEngine embeddings_engine;
    {
        Timer t("Step 6: Loading Word Embeddings (THIS MAY BE SLOW)");
        if (std::filesystem::exists(binary_embedding_file)) {
            if (!embeddings_engine.load_embeddings_binary(binary_embedding_file)) {
                std::cerr << "Error: Failed to load embeddings!" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Warning: Binary embedding file not found. Skipping embeddings." << std::endl;
        }
    }
    
    // =================== Step 7: Initialize Semantic Search Engine ===================
    ParallelSemanticSearch semantic_engine(&embeddings_engine, &bm25_engine);
    semantic_engine.set_weights(0.6f, 0.4f);
    
    // =================== Step 8: Initialize AutoComplete (POTENTIAL BOTTLENECK) ===================
    AutoComplete autocomplete(10, false, 2);
     if (std::filesystem::exists(autocomplete_cache)) {
        std::cout << "\n[FAST LOAD] Loading pre-built autocomplete..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        if (autocomplete.load_from_binary(autocomplete_cache)) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "Loaded autocomplete in " << duration.count() << " ms (FAST!)" << std::endl;
        } else {
            std::cout << "Failed to load cache, rebuilding..." << std::endl;
            goto rebuild;
        }
    } else {
        rebuild:
        std::cout << "\n[SLOW BUILD] Building autocomplete for first time..." << std::endl;
        std::cout << "This will take 5-15 seconds but only happens once..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        autocomplete.initialize_from_lexicon(&lexicon);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Built in " << duration.count() << " ms" << std::endl;
        
        // Save for next time
        std::cout << "Saving cache for fast loading next time..." << std::endl;
        autocomplete.save_to_binary(autocomplete_cache);
    }
    autocomplete.print_statistics();
    std::cout << "\nAll systems ready!\n" << std::endl;
    
    // =================== Interactive Search Loop ===================
    std::cout << std::string(100, '=') << std::endl;
    std::cout << "COVID-19 RESEARCH PAPER SEARCH ENGINE" << std::endl;
    std::cout << "Searching through " << forward_index.get_forward_index().size() 
              << " research papers" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    std::string query;
    int search_mode = 0;
    
    while (true) {
        std::cout << "\n" << std::string(100, '-') << std::endl;
        std::cout << "SEARCH OPTIONS:" << std::endl;
        std::cout << "[1] BM25 Keyword Search" << std::endl;
        std::cout << "[2] Semantic Search" << std::endl;
        std::cout << "[3] Hybrid Search (Recommended)" << std::endl;
        std::cout << "[4] Test AutoComplete" << std::endl;
        std::cout << "[0] Exit" << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        std::cout << "Select (0-4): ";
        
        if (!(std::cin >> search_mode)) {
            break;
        }
        std::cin.ignore();
        
        if (search_mode == 0) break;
        
        // =================== AutoComplete Demo ===================
        if (search_mode == 4) {
            std::cout << "\nEnter prefix for autocomplete: ";
            std::string prefix;
            std::getline(std::cin, prefix);
            
            auto ac_start = std::chrono::high_resolution_clock::now();
            auto suggestions = autocomplete.get_suggestions(prefix, 10);
            auto ac_end = std::chrono::high_resolution_clock::now();
            auto ac_duration = std::chrono::duration_cast<std::chrono::microseconds>(ac_end - ac_start);
            
            std::cout << "\nSuggestions for '" << prefix << "':" << std::endl;
            for (size_t i = 0; i < suggestions.size(); i++) {
                std::cout << "  " << (i+1) << ". " << suggestions[i].word 
                         << " (freq: " << suggestions[i].frequency << ")" << std::endl;
            }
            std::cout << "Time: " << ac_duration.count() << " microsec" << std::endl;
            continue;
        }
        
        if (search_mode < 1 || search_mode > 3) {
            std::cout << "Invalid option." << std::endl;
            continue;
        }
        
        std::cout << "\nEnter search query: ";
        if (!std::getline(std::cin, query) || query.empty()) {
            continue;
        }
        
        // Extract last word for autocomplete suggestion
        std::istringstream iss(query);
        std::string word, last_word;
        while (iss >> word) last_word = word;
        
        if (last_word.length() >= 2) {
            auto ac_start = std::chrono::high_resolution_clock::now();
            auto suggestions = autocomplete.get_suggestions(last_word, 5);
            auto ac_end = std::chrono::high_resolution_clock::now();
            auto ac_ms = std::chrono::duration_cast<std::chrono::microseconds>(ac_end - ac_start);
            
            if (!suggestions.empty()) {
                std::cout << "Suggestions (" << ac_ms.count() << " microsec): ";
                for (size_t i = 0; i < std::min(size_t(5), suggestions.size()); i++) {
                    std::cout << suggestions[i].word << " ";
                }
                std::cout << "\n" << std::endl;
            }
        }
        
        // Preprocess query
        auto prep_start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> query_tokens = preprocessor.preprocess(query);
        auto prep_end = std::chrono::high_resolution_clock::now();
        auto prep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(prep_end - prep_start);
        
        std::cout << "Query preprocessing: " << prep_ms.count() << " ms" << std::endl;
        
        if (query_tokens.empty()) {
            std::cout << "Warning: No valid tokens" << std::endl;
            continue;
        }
        
        std::cout << "Tokens: ";
        for (const auto& token : query_tokens) {
            std::cout << token << " ";
        }
        std::cout << "\n" << std::endl;
        
        // Perform search with detailed timing
        auto search_start = std::chrono::high_resolution_clock::now();
        
        if (search_mode == 1) {
            std::cout << "[BM25 Search]" << std::endl;
            auto results = bm25_engine.search(query, 10);
            bm25_engine.print_results(results);
            
        } else if (search_mode == 2) {
            std::cout << "[Semantic Search]" << std::endl;
            auto results = semantic_engine.semantic_search(query, query_tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
            
        } else if (search_mode == 3) {
            std::cout << "[Hybrid Search]" << std::endl;
            auto results = semantic_engine.hybrid_search(query, query_tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
        }
        
        auto search_end = std::chrono::high_resolution_clock::now();
        auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);
        
        std::cout << "\nTotal search time: " << search_duration.count() << " ms" << std::endl;
        
        if (search_duration.count() <= 500) {
            std::cout << "EXCELLENT: Under 500ms" << std::endl;
        } else if (search_duration.count() <= 1500) {
            std::cout << "GOOD: Under 1500ms" << std::endl;
        } else {
            std::cout << "SLOW: Over 1500ms - optimization needed" << std::endl;
        }
    }
    
    std::cout << "\nThank you for using the search engine!" << std::endl;
    return 0;
}