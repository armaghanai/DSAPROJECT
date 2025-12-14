#include "../include/SearchEngine.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"
#include "../include/TextPreProcessor.hpp"

#include <iostream>
#include <string>

int main() {
    // =================== Configuration ===================
    std::string indices_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    
    std::cout << "=== Loading Search Engine (2000 Documents) ===" << std::endl;
    
    // =================== Step 1: Load Lexicon ===================
    std::cout << "\n[1/4] Loading Lexicon..." << std::endl;
    LexiconBuilder lexicon;
    if (!lexicon.load_from_csv(indices_path + "lexicon.csv")) {
        std::cerr << "Error: Failed to load lexicon!" << std::endl;
        return 1;
    }
    std::cout << "✓ Lexicon loaded: " << lexicon.get_size() << " words" << std::endl;
    
    // =================== Step 2: Load Forward Index ===================
    std::cout << "\n[2/4] Loading Forward Index..." << std::endl;
    ForwardIndex forward_index;
    if (!forward_index.load_from_binary(indices_path + "forward_index.bin")) {
        std::cerr << "Error: Failed to load forward index!" << std::endl;
        return 1;
    }
    std::cout << "✓ Forward index loaded" << std::endl;
    forward_index.print_statistics();
    
    // =================== Step 3: Load Inverted Index (Barrels) ===================
    std::cout << "\n[3/4] Loading Inverted Index Barrels..." << std::endl;
    InvertedIndex inverted_index;
    if (!inverted_index.load_barrel_metadata(barrel_path)) {
        std::cerr << "Error: Failed to load barrel metadata!" << std::endl;
        return 1;
    }
    std::cout << "✓ Barrel metadata loaded" << std::endl;
    inverted_index.print_barrel_info();
    
    // =================== Step 4: Initialize Search Engine ===================
    std::cout << "\n[4/4] Initializing Search Engine..." << std::endl;
    TextPreprocessor preprocessor;
    SearchEngine search_engine(&lexicon, &forward_index, &inverted_index, &preprocessor);
    
    // =================== Interactive Search Loop ===================
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "COVID-19 RESEARCH PAPER SEARCH ENGINE" << std::endl;
    std::cout << "Searching through 2000 research papers" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    while (true) {
        std::cout << "\nEnter your search query (or 'quit' to exit): ";
        std::string query;
        std::getline(std::cin, query);
        
        if (query == "quit" || query == "exit" || query == "q") {
            std::cout << "Exiting search engine. Goodbye!" << std::endl;
            break;
        }
        
        if (query.empty()) {
            continue;
        }
        
        // Perform search
        auto results = search_engine.search(query, 10); // Get top 10 results
        
        // Display results
        search_engine.print_results(results);
        
        // Ask if user wants to see full document
        if (!results.empty()) {
            std::cout << "\nEnter result number to view full details (or press Enter to continue): ";
            std::string choice;
            std::getline(std::cin, choice);
            
            if (!choice.empty()) {
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(results.size())) {
                        const auto& result = results[idx];
                        const DocumentIndex* doc = search_engine.get_document(result.doc_id);
                        
                        if (doc) {
                            std::cout << "\n" << std::string(80, '=') << std::endl;
                            std::cout << "FULL DOCUMENT DETAILS" << std::endl;
                            std::cout << std::string(80, '=') << std::endl;
                            std::cout << "Title: " << doc->title << std::endl;
                            std::cout << "Doc ID: " << doc->doc_id << std::endl;
                            std::cout << "Document Length: " << doc->doc_length << " terms" << std::endl;
                            std::cout << "\nAbstract:\n" << doc->abstract_text << std::endl;
                            std::cout << "\n" << std::string(80, '=') << std::endl;
                        }
                    }
                } catch (...) {
                    std::cout << "Invalid selection." << std::endl;
                }
            }
        }
    }
    
    return 0;
}