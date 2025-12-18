// main_api.cpp - Bulletproof version that silences ALL stdout except JSON
#include "../include/CachedSearchEngine.hpp"
#include "../include/SemanticSearchEngine.hpp"
#include "../include/WordEmbeddingsEngine.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/AutoComplete.hpp"
#include "../include/metadataparser.hpp"
#include "../include/Lemmatizer.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstring>

int main() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
std::string data_path = "D:/THird Semester/DSA/dsaspp/DSAPROJECT/data/2020-04-10";   
 std::string indices_path = "D:/THird Semester/DSA/dsaspp/DSAPROJECT/indices/";
    
    std::cout << "=== CORD-19 Lexicon Builder ===" << std::endl;
    std::cout << "Data path: " << data_path << std::endl;
    std::cout << "Output path: " << indices_path << std::endl << std::endl;
    
    // Step 1: Parse metadata
    std::cout << "Step 1: Parsing metadata..." << std::endl;
    MetadataParser parser(data_path);
    
    int total_papers = parser.metadata_stats();
    std::cout << "Found " << total_papers << " papers in metadata" << std::endl;
    
    int parsed = parser.metadata_parse();
    std::cout << "Successfully parsed " << parsed << " papers" << std::endl << std::endl;
    
    // Step 2: Initialize lemmatizer and lexicon builder
    std::cout << "Step 2: Initializing lemmatizer and lexicon builder..." << std::endl;
    Lemmatizer lemmatizer;
    LexiconBuilder lexicon;
    std::cout << std::endl;
    
    // Step 3: Process papers and build lexicon
    std::cout << "Step 3: Processing papers and building lexicon..." << std::endl;
    
    const auto& papers = parser.getPapers();
    int papers_processed = 0;
    int papers_with_text = 0;
    
    for (const auto& paper : papers) {
        // Combine abstract and body text
        std::string full_text = paper.abstract_text;
        
        if (!paper.body_text.empty()) {
            full_text += " " + paper.body_text;
            papers_with_text++;
        }
        
        if (full_text.empty()) {
            continue;
        }
        
        // Get term frequencies from lemmatizer
        std::unordered_map<std::string, u32> term_freqs;
        lemmatizer.process_text(full_text, term_freqs);
        
        // Add terms to lexicon
        for (const auto& [term, freq] : term_freqs) {
            lexicon.add_word(term, freq);
        }
        
        papers_processed++;
        
        // Progress update every 1000 papers
        if (papers_processed % 1000 == 0) {
            std::cout << "Processed " << papers_processed << " papers, "
                      << "Lexicon size: " << lexicon.get_size() << " unique terms" << std::endl;
        }
    }
    
    std::cout << "\nProcessing complete!" << std::endl;
    std::cout << "Total papers processed: " << papers_processed << std::endl;
    std::cout << "Papers with full text: " << papers_with_text << std::endl;
    std::cout << "Final lexicon size: " << lexicon.get_size() << " unique terms" << std::endl << std::endl;
    
    // Step 4: Save lexicon to CSV
    std::cout << "Step 4: Saving lexicon to CSV..." << std::endl;
    std::string lexicon_path = indices_path + "lexicon.csv";
    lexicon.save_to_csv(lexicon_path);
    std::cout << "✓ Lexicon saved to: " << lexicon_path << std::endl << std::endl;
    
    // Step 5: Show statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    std::cout << "=== Statistics ===" << std::endl;
    std::cout << "Time taken: " << duration.count() << " seconds" << std::endl;
    std::cout << "Papers processed: " << papers_processed << std::endl;
    std::cout << "Unique terms in lexicon: " << lexicon.get_size() << std::endl;
    
    // Show top 10 most frequent terms
    auto lexicon_data = lexicon.get_lexicon_data();
    std::vector<std::pair<std::string, std::pair<uint32_t, uint32_t>>> sorted_terms(
        lexicon_data.begin(), lexicon_data.end()
    );
    
    std::sort(sorted_terms.begin(), sorted_terms.end(),
              [](const auto& a, const auto& b) { return a.second.second > b.second.second; });
    
    std::cout << "\nTop 10 most frequent terms:" << std::endl;
    for (int i = 0; i < 10 && i < sorted_terms.size(); i++) {
        std::cout << (i+1) << ". " << sorted_terms[i].first 
                  << " (freq: " << sorted_terms[i].second.second << ")" << std::endl;
    }
    
    std::cout << "\n✓ Lexicon building complete!" << std::endl;
    
    return 0;
}