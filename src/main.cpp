#include "../include/SearchEngine.hpp"
#include "../include/SemanticSearchEngine.hpp"
#include "../include/WordEmbeddingsEngine.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"
#include "../include/TextPreProcessor.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>

int main() {
    // =================== Configuration ===================
    std::string indices_path = "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    std::string binary_embedding_file = 
    "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\embeddings\\glove.6B\\glove.6B.300d.bin";
    
    // Verify paths exist
    if (!std::filesystem::exists(indices_path)) {
        std::cerr << "Error: indices directory not found" << std::endl;
        return 1;
    }
    
    std::cout << "=== COVID-19 Research Paper Search Engine ===" << std::endl;
    std::cout << "With BM25 Keyword + Semantic Search" << std::endl;
    std::cout << "==========================================\n" << std::endl;
    
    // =================== Step 1: Load Lexicon ===================
    std::cout << "[1/6] Loading Lexicon..." << std::endl;
    LexiconBuilder lexicon;
    if (!lexicon.load_from_csv(indices_path + "lexicon.csv")) {
        std::cerr << "Error: Failed to load lexicon!" << std::endl;
        return 1;
    }
    std::cout << "✓ Loaded " << lexicon.get_size() << " words\n" << std::endl;
    
    auto reverse_lex = lexicon.build_reverse_lexicon();
    
    // =================== Step 2: Load Forward Index ===================
    std::cout << "[2/6] Loading Forward Index..." << std::endl;
    ForwardIndex forward_index;
    if (!forward_index.load_from_binary(indices_path + "forward_index.bin")) {
        std::cerr << "Error: Failed to load forward index!" << std::endl;
        return 1;
    }
    forward_index.print_statistics();
    
    // =================== Step 3: Load Inverted Index ===================
    std::cout << "[3/6] Loading Inverted Index..." << std::endl;
    InvertedIndex inverted_index;
    if (!inverted_index.load_from_binary(indices_path + "inverted_index.bin", reverse_lex)) {
        std::cerr << "Error: Failed to load inverted index!" << std::endl;
        return 1;
    }
    inverted_index.print_statistics();
    
    // =================== Step 4: Setup Barrels ===================
    std::cout << "[4/6] Setting up Barrels..." << std::endl;
    if (!std::filesystem::exists(barrel_path)) {
        std::cout << "Creating new barrels..." << std::endl;
        std::cout << "Number of barrels (recommended: 10-20): ";
        
        uint32_t num_barrels;
        std::cin >> num_barrels;
        std::cin.ignore();
        
        if (num_barrels < 1 || num_barrels > 100) {
            std::cout << "Invalid input. Using default: 10" << std::endl;
            num_barrels = 10;
        }
        
        if (!inverted_index.create_barrels(barrel_path, reverse_lex, num_barrels)) {
            std::cerr << "Error: Failed to create barrels!" << std::endl;
            return 1;
        }
    }
    
    if (!inverted_index.load_barrel_metadata(barrel_path)) {
        std::cerr << "Error: Failed to load barrel metadata!" << std::endl;
        return 1;
    }
    std::cout << "✓ Barrels loaded\n" << std::endl;
    
    // =================== Step 5: Initialize BM25 Search Engine ===================
    std::cout << "[5/6] Initializing BM25 Search Engine..." << std::endl;
    TextPreprocessor preprocessor;
    SearchEngine bm25_engine(&lexicon, &forward_index, &inverted_index, &preprocessor);
    
    // =================== Step 6: Load Word Embeddings (Binary Format) ===================
    std::cout << "[6/6] Loading Word Embeddings..." << std::endl;
    
    if (!std::filesystem::exists(binary_embedding_file)) {
        std::cerr << "\nERROR: Binary embedding file not found!" << std::endl;
        std::cerr << "Expected: " << binary_embedding_file << std::endl;
        return 1;
    }
    
    WordEmbeddingsEngine embeddings_engine;
    
    std::cout << "Loading embeddings from: " << binary_embedding_file << std::endl;
    auto emb_start = std::chrono::high_resolution_clock::now();
    
    if (!embeddings_engine.load_embeddings_binary(binary_embedding_file)) {
        std::cerr << "Error: Failed to load embeddings!" << std::endl;
        return 1;
    }
    
    auto emb_end = std::chrono::high_resolution_clock::now();
    auto emb_duration = std::chrono::duration_cast<std::chrono::milliseconds>(emb_end - emb_start);
    std::cout << "✓ Embeddings loaded in " << emb_duration.count() << " ms\n" << std::endl;
    
    // =================== Initialize Semantic Search Engine ===================
    std::cout << "[7/7] Initializing Semantic Search Engine..." << std::endl;
    SemanticSearchEngine semantic_engine(&embeddings_engine, &bm25_engine);
    
    // Set hybrid search weights (60% semantic, 40% BM25)
    semantic_engine.set_weights(0.6f, 0.4f);
    std::cout << "✓ All engines ready!\n" << std::endl;
    
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
        std::cout << "[1] BM25 Keyword Search (Fast, Exact Match)" << std::endl;
        std::cout << "[2] Semantic Search (Meaning-Based)" << std::endl;
        std::cout << "[3] Hybrid Search (BM25 + Semantic - Recommended)" << std::endl;
        std::cout << "[0] Exit" << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        std::cout << "Select search mode (0-3): ";
        
        if (!(std::cin >> search_mode)) {
            break;
        }
        std::cin.ignore();
        
        if (search_mode == 0) {
            std::cout << "\nThank you for using the search engine!" << std::endl;
            break;
        }
        
        if (search_mode < 1 || search_mode > 3) {
            std::cout << "Invalid option. Please select 0-3." << std::endl;
            continue;
        }
        
        std::cout << "\nEnter your search query: ";
        if (!std::getline(std::cin, query) || query.empty()) {
            std::cout << "Empty query. Try again." << std::endl;
            continue;
        }
        
        // Preprocess query
        std::vector<std::string> query_tokens = preprocessor.preprocess(query);
        
        if (query_tokens.empty()) {
            std::cout << "Warning: Query produced no valid tokens after preprocessing." << std::endl;
            std::cout << "Try using different words." << std::endl;
            continue;
        }
        
        std::cout << "\nQuery tokens: ";
        for (const auto& token : query_tokens) {
            std::cout << token << " ";
        }
        std::cout << "\n" << std::endl;
        
        // Perform search with timing
        auto search_start = std::chrono::high_resolution_clock::now();
        
        if (search_mode == 1) {
            // BM25 Search
            std::cout << "Performing BM25 keyword search..." << std::endl;
            auto results = bm25_engine.search(query, 10);
            bm25_engine.print_results(results);
            
        } else if (search_mode == 2) {
            // Semantic Search
            std::cout << "Performing semantic search (meaning-based)..." << std::endl;
            auto results = semantic_engine.semantic_search(query, query_tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
            
        } else if (search_mode == 3) {
            // Hybrid Search
            std::cout << "Performing hybrid search (BM25 + Semantic)..." << std::endl;
            auto results = semantic_engine.hybrid_search(query, query_tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
        }
        
        auto search_end = std::chrono::high_resolution_clock::now();
        auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);
        
        std::cout << "Search completed in " << search_duration.count() << " ms" << std::endl;
        
        // Check if meets performance requirements
        if (search_duration.count() <= 500) {
            std::cout << "EXCELLENT: Under 500ms requirement" << std::endl;
        } else if (search_duration.count() <= 1500) {
            std::cout << "GOOD: Under 1500ms requirement" << std::endl;
        } else {
            std::cout << "WARNING: Exceeds performance target" << std::endl;
        }
        
        // Optional: View full document details
        std::cout << "\nView full document details? (enter number or press Enter to skip): ";
        std::string choice;
        if (std::getline(std::cin, choice) && !choice.empty()) {
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < 10) {
                    // Try to get the document
                    // Note: Results are local to the search block above
                    // For this to work, you'd need to modify the structure to keep results
                    std::cout << "(Document details would be shown here)" << std::endl;
                } else {
                    std::cout << "Invalid selection." << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid input." << std::endl;
            }
        }
    }
    
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << "Program terminated. Goodbye!" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    return 0;
}