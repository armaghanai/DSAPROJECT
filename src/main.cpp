#include <iostream>
#include <string>
#include <filesystem>
#include "../include/metadataparser.hpp"
#include "../include/LexiconBuilder.hpp"

namespace fs = std::filesystem;

int main() {
    std::cout << "=== LEXICON TEXT ===" << std::endl;

    std::cout << "Current directory: " << fs::current_path() << "\n" << std::endl;
    
    // Path from build/ directory
    std::string path = "../data/2020-04-10/metadata.csv";
    
    // Check if file exists
    std::cout << "Looking for file at: " << path << std::endl;
    if (!fs::exists(path)) {
        std::cerr << "ERROR: File not found!" << std::endl;
        
        // Show absolute path for debugging
        fs::path abs_path = fs::absolute(path);
        std::cerr << "Absolute path would be: " << abs_path << std::endl;
        return 1;
    }
    
    std::cout << "✓ File found!" << std::endl;
    std::cout << "File size: " << fs::file_size(path) << " bytes\n" << std::endl;
    
    // Parse metadata
    std::cout << "Starting to parse..." << std::endl;
    MetadataParser parser(path);

    if (!parser.parse()) {
        std::cerr << "✗ Parsing failed!" << std::endl;
        return 1;
    }

    /*

    // Get papers
    const auto &papers = parser.getPapers();
    
    if (papers.empty()) {
        std::cerr << "Warning: No papers found!" << std::endl;
        return 1;
    }
    
    // Print first 3 papers
    std::cout << "=== FIRST 3 PAPERS ===" << std::endl;
    int count = std::min(3, static_cast<int>(papers.size()));
    
    for (int i = 0; i < count; i++) {
        std::cout << "\n--- PAPER " << (i+1) << " ---" << std::endl;
        std::cout << "CORD UID: " << papers[i].cord_uid << std::endl;
        std::cout << "Title: " << papers[i].title << std::endl;
        std::cout << "Authors: " << papers[i].authors << std::endl;
        std::cout << "Journal: " << papers[i].journal << std::endl;
        std::cout << "Publish Date: " << papers[i].publish_time << std::endl;
        
        if (!papers[i].abstract_text.empty()) {
            size_t len = std::min(static_cast<size_t>(200), papers[i].abstract_text.length());
            std::cout << "Abstract: " << papers[i].abstract_text.substr(0, len);
            if (papers[i].abstract_text.length() > 200) {
                std::cout << "...";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Abstract: [No abstract]" << std::endl;
        }
    }
    
    std::cout << "\n=== TEST COMPLETED ===" << std::endl;*/
    
    LexiconBuilder lexicon;
    for (auto& paper : parser.getPapers()) {
        std::string text = paper.title + " " + paper.abstract_text;
        lexicon.addDocument(paper.cord_uid, text);
    }

    lexicon.build();
    lexicon.displayLexicon();


    return 0;
}