#include "../include/metadataparser.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main() {
    // Step 1: Set dataset path
    std::string dataset_path = "D:/THird Semester/DSA/dsaspp/DSAPROJECT/data/2020-04-10";
    std::string indices_path = "D:/THird Semester/DSA/dsaspp/DSAPROJECT/indices/";
    
    MetadataParser parser(dataset_path);

    // Step 2: Parse metadata
    std::cout << "Counting metadata entries..." << std::endl;
    int total_entries = parser.metadata_stats();
    std::cout << "Total metadata entries: " << total_entries << std::endl;

    std::cout << "\nParsing metadata and extracting papers..." << std::endl;
    parser.metadata_parse();
    std::cout << "Total papers parsed: " << parser.papers.size() << std::endl;

    // Step 3: Text preprocessing setup
    TextPreprocessor preprocessor;

    // Step 4: Build lexicon
    std::cout << "\n=== Building Lexicon ===" << std::endl;
    LexiconBuilder lexicon;

    for (const auto& paper : parser.papers) {
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        
        for (const auto& token : tokens) {
            lexicon.add_word(token, 1);
        }
    }

    std::cout << "Lexicon size: " << lexicon.get_size() << " unique words" << std::endl;

    // Step 5: Save lexicon to CSV
    std::string lexicon_file = indices_path + "lexicon.csv";
    lexicon.save_to_csv(lexicon_file);
    std::cout << "Lexicon saved to " << lexicon_file << "\n";

    // Step 6: Build Forward Index
    std::cout << "\n=== Building Forward Index ===" << std::endl;
    ForwardIndex forward_index;
    
    int processed_docs = 0;
    for (const auto& paper : parser.papers) {
        // Preprocess the text to get tokens
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        
        // Convert tokens to word IDs using the lexicon
        std::vector<uint32_t> word_ids;
        word_ids.reserve(tokens.size());
        
        for (const auto& token : tokens) {
            uint32_t word_id = lexicon.get_word_id(token);
            if (word_id != UINT32_MAX) {  // Valid word ID
                word_ids.push_back(word_id);
            }
        }
        
        // Add document to forward index
        if (!word_ids.empty()) {
            forward_index.add_document(
                paper.paper_id,
                paper.title,
                paper.abstract_text,
                word_ids
            );
            processed_docs++;
            
            if (processed_docs % 1000 == 0) {
                std::cout << "Processed " << processed_docs << " documents..." << std::endl;
            }
        }
    }
    
    std::cout << "\nForward index built successfully!" << std::endl;
    forward_index.print_statistics();
    
    // Step 7: Save Forward Index
    std::string forward_index_binary = indices_path + "forward_index.bin";
    std::string forward_index_csv = indices_path + "forward_index.csv";
    
    std::cout << "Saving forward index..." << std::endl;
    forward_index.save_to_binary(forward_index_binary);
    
    // Optionally save to CSV (can be large!)
    std::cout << "\nDo you want to save forward index to CSV? (Warning: This can be very large!) [y/n]: ";
    char choice;
    std::cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        forward_index.save_to_csv(forward_index_csv);
    }
    
    // Step 8: Test loading forward index
    std::cout << "\n=== Testing Forward Index Loading ===" << std::endl;
    ForwardIndex test_index;
    
    if (test_index.load_from_binary(forward_index_binary)) {
        std::cout << "Forward index loaded successfully!" << std::endl;
        test_index.print_statistics();
        
        // Example: Query first document
        if (!parser.papers.empty()) {
            const std::string& first_doc_id = parser.papers[0].paper_id;
            std::cout << "\n=== Sample Document Query ===" << std::endl;
            std::cout << "Querying document: " << first_doc_id << std::endl;
            
            const DocumentIndex* doc = test_index.get_document(first_doc_id);
            if (doc) {
                std::cout << "Title: " << doc->title << std::endl;
                std::cout << "Abstract: " << doc->abstract_text.substr(0, 200) << "..." << std::endl;
                std::cout << "Document Length: " << doc->doc_length << " terms" << std::endl;
                std::cout << "Unique Terms: " << doc->terms.size() << std::endl;
                
                // Show first 5 terms
                std::cout << "\nFirst 5 terms in document:" << std::endl;
                for (size_t i = 0; i < std::min(size_t(5), doc->terms.size()); ++i) {
                    std::cout << "  Word ID: " << doc->terms[i].word_id 
                              << ", Frequency: " << doc->terms[i].frequency << std::endl;
                }
            }
        }
    } else {
        std::cout << "Error loading forward index!" << std::endl;
    }

    std::cout << "\n=== Indexing Complete ===" << std::endl;
    return 0;
}