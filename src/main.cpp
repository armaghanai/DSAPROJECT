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
    std::string dataset_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\data\\2020-04-10";
    std::string indices_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    
    const int MAX_DOCS = 2000;  // Only process 2000 documents

    // =================== Step 1: Parse Metadata ===================
    std::cout << "=== Parsing Metadata ===" << std::endl;
    MetadataParser parser(dataset_path);
    parser.metadata_parse();
    
    // Get only first 2000 papers
    auto all_papers = parser.getPapers();
    int actual_limit = std::min(MAX_DOCS, (int)all_papers.size());
    std::vector<Paper> papers_subset(all_papers.begin(), 
                                     all_papers.begin() + actual_limit);
    
    std::cout << "Total papers parsed: " << all_papers.size() << std::endl;
    std::cout << "Using only first " << papers_subset.size() << " documents for indexing" << std::endl;

    // =================== Step 2: Build Lexicon (2000 docs only) ===================
    std::cout << "\n=== Building Lexicon from " << papers_subset.size() << " documents ===" << std::endl;
    TextPreprocessor preprocessor;
    LexiconBuilder lexicon;
    
    for (const auto& paper : papers_subset) {
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        for (const auto& token : tokens) {
            lexicon.add_word(token, 1);
        }
    }
    
    lexicon.save_to_csv(indices_path + "lexicon.csv");
    std::cout << "Lexicon size: " << lexicon.get_size() << " unique words" << std::endl;

    // =================== Step 3: Build Forward Index (2000 docs only) ===================
    std::cout << "\n=== Building Forward Index ===" << std::endl;
    ForwardIndex forward_index;
    int processed_docs = 0;
    
    for (const auto& paper : papers_subset) {
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        std::vector<uint32_t> word_ids;
        word_ids.reserve(tokens.size());

        for (const auto& token : tokens) {
            uint32_t word_id = lexicon.get_word_id(token);
            if (word_id != UINT32_MAX) {
                word_ids.push_back(word_id);
            }
        }

        if (!word_ids.empty()) {
            forward_index.add_document(paper.paper_id, paper.title, paper.abstract_text, word_ids);
            processed_docs++;
            if (processed_docs % 500 == 0) {
                std::cout << "Processed " << processed_docs << " documents..." << std::endl;
            }
        }
    }
    
    std::cout << "Forward index built successfully!" << std::endl;
    forward_index.print_statistics();
    forward_index.save_to_binary(indices_path + "forward_index.bin");

    // =================== Step 4: Build Inverted Index (2000 docs only) ===================
    std::cout << "\n=== Building Inverted Index ===" << std::endl;
    std::unordered_map<uint32_t, std::string> reverse_lex = lexicon.build_reverse_lexicon();
    InvertedIndex inverted_index;

    for (const auto& [doc_id_str, doc_num_id] : forward_index.get_doc_id_map()) {
        const DocumentIndex* doc = forward_index.get_document(doc_id_str);
        if (!doc) continue;

        std::vector<std::pair<uint32_t, uint32_t>> terms;
        for (const auto& t : doc->terms) {
            terms.emplace_back(t.word_id, t.frequency);
        }
        inverted_index.add_document(doc_num_id, terms);
    }

    inverted_index.save_to_binary(indices_path + "inverted_index.bin", reverse_lex);
    inverted_index.print_statistics();

    // =================== Step 5: Create Barrels (2000 docs) ===================
    std::cout << "\n=== Creating Barrels ===" << std::endl;
    inverted_index.create_barrels(barrel_path, reverse_lex, 4);
    inverted_index.print_barrel_info();

    // =================== Step 6: Export Barrels to CSV for Submission ===================
    std::cout << "\n=== Exporting Barrels to CSV ===" << std::endl;
    
    // Load metadata and export each barrel to CSV
    InvertedIndex export_idx;
    export_idx.load_barrel_metadata(barrel_path);
    
    for (int i = 0; i < 4; i++) {
        // Load barrel i
        uint32_t test_word_id = i * (lexicon.get_size() / 4);  // Get a word from this barrel
        export_idx.load_barrel_for_word(test_word_id, reverse_lex);
        
        // Export to CSV
        std::string csv_path = barrel_path + "/inverted_barrel_" + std::to_string(i) + ".csv";
        std::ofstream csv_out(csv_path);
        csv_out << "word_id,word,doc_id,frequency\n";
        
        auto barrel_data = export_idx.get_inverted_index();
        for (const auto& [word_id, postings] : barrel_data) {
            std::string word = reverse_lex[word_id];
            for (const auto& [doc_id, freq] : postings) {
                csv_out << word_id << "," << word << "," << doc_id << "," << freq << "\n";
            }
        }
        csv_out.close();
        std::cout << "Exported Barrel " << i << " to CSV (" << barrel_data.size() << " words)" << std::endl;
    }

    // =================== Step 7: Test Barrel Queries ===================
    std::cout << "\n=== Testing Barrel Queries ===" << std::endl;
    
    InvertedIndex query_idx;
    query_idx.load_barrel_metadata(barrel_path);
    query_idx.print_barrel_info();

    // Test with actual words from lexicon
    std::vector<std::string> test_words = {"virus", "infection", "cells", "protein", "patients"};
    
    std::cout << "\n=== Testing Words ===" << std::endl;
    for (const auto& word : test_words) {
        uint32_t word_id = lexicon.get_word_id(word);
        
        if (word_id == UINT32_MAX) {
            std::cout << "Word '" << word << "' not found in lexicon" << std::endl;
            continue;
        }
        
        std::cout << "\n--- Searching: '" << word << "' (ID: " << word_id << ") ---" << std::endl;
        query_idx.load_barrel_for_word(word_id, reverse_lex);
        
        const auto* postings = query_idx.get_terms(word_id);
        
        if (postings) {
            std::cout << "Found in " << postings->size() << " documents" << std::endl;
            
            size_t count = 0;
            for (const auto& [doc_id, freq] : *postings) {
                std::cout << "  Doc " << doc_id << ": " << freq << " times" << std::endl;
                if (++count >= 5) break;
            }
        }
    }

    std::cout << "\n=== Processing Complete for " << papers_subset.size() << " documents ===" << std::endl;
    return 0;
}