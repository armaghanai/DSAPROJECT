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
    // =================== Step 1: Dataset paths ===================
    std::string dataset_path =
        "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\data\\2020-04-10";
    std::string indices_path =
        "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\indices\\";

    // =================== Step 2: Parse metadata ===================
    MetadataParser parser(dataset_path);
    std::cout << "Counting metadata entries..." << std::endl;
    int total_entries = parser.metadata_stats();
    std::cout << "Total metadata entries: " << total_entries << std::endl;

    std::cout << "\nParsing metadata and extracting papers..." << std::endl;
    parser.metadata_parse();
    std::cout << "Total papers parsed: " << parser.getCount() << std::endl;

    // =================== Step 3: Preprocessor ===================
    TextPreprocessor preprocessor;

    // =================== Step 4: Build Lexicon ===================
    std::cout << "\n=== Building Lexicon ===" << std::endl;
    LexiconBuilder lexicon;
    for (const auto& paper : parser.getPapers()) {
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        for (const auto& token : tokens) {
            lexicon.add_word(token, 1);
        }
    }
    std::cout << "Lexicon size: " << lexicon.get_size() << " unique words" << std::endl;

    // =================== Step 5: Build Forward Index ===================
    std::cout << "\n=== Building Forward Index ===" << std::endl;
    ForwardIndex forward_index;
    int processed_docs = 0;
    for (const auto& paper : parser.getPapers()) {
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);
        std::vector<uint32_t> word_ids;
        word_ids.reserve(tokens.size());

        for (const auto& token : tokens) {
            uint32_t word_id = lexicon.get_word_id(token);
            if (word_id != UINT32_MAX) word_ids.push_back(word_id);
        }

        if (!word_ids.empty()) {
            forward_index.add_document(paper.paper_id, paper.title, paper.abstract_text, word_ids);
            processed_docs++;
            if (processed_docs % 1000 == 0)
                std::cout << "Processed " << processed_docs << " documents..." << std::endl;
        }
    }
    std::cout << "\nForward index built successfully!" << std::endl;
    forward_index.print_statistics();

    // =================== Step 6: Save first 1000 docs to CSV ===================
    forward_index.save_first_n_to_csv(indices_path + "forward_index.csv", 1000);

    // =================== Step 7: Save full forward index to binary ===================
    forward_index.save_to_binary(indices_path + "forward_index.bin");

    // =================== Step 8: Load forward index from binary ===================
    std::cout << "\n=== Testing Forward Index Loading ===" << std::endl;
    ForwardIndex test_index;
    if (test_index.load_from_binary(indices_path + "forward_index.bin")) {
        std::cout << "Forward index loaded successfully!" << std::endl;
        test_index.print_statistics();

        // =================== Step 9: Sample document query ===================
        size_t sample_index = 0; // pick first document
        if (!test_index.get_doc_id_map().empty()) {
            auto it = test_index.get_doc_id_map().begin();
            std::advance(it, sample_index);
            const std::string& doc_id = it->first;

            const DocumentIndex* doc = test_index.get_document(doc_id);
            if (doc) {
                std::cout << "\n=== Sample Document Query ===" << std::endl;
                std::cout << "Querying document: " << doc_id << std::endl;
                std::cout << "Title: " << doc->title << std::endl;
                std::cout << "Abstract (first 200 chars): " 
                          << doc->abstract_text.substr(0, std::min<size_t>(200, doc->abstract_text.size())) << "..." << std::endl;
                std::cout << "Document Length: " << doc->doc_length << " terms" << std::endl;
                std::cout << "Unique Terms: " << doc->terms.size() << std::endl;

                size_t display_terms = std::min<size_t>(5, doc->terms.size());
                std::cout << "\nFirst " << display_terms << " terms in document:" << std::endl;
                for (size_t i = 0; i < display_terms; ++i) {
                    std::cout << "  Word ID: " << doc->terms[i].word_id
                              << ", Frequency: " << doc->terms[i].frequency << std::endl;
                }
            }
        }
    } else {
        std::cout << "Error loading forward index!" << std::endl;
    }

    // =================== Step 10: Build Inverted Index ===================
    std::unordered_map<uint32_t, std::string> reverse_lex = lexicon.build_reverse_lexicon();
    InvertedIndex inverted_index;

    // Use numeric doc_id from forward_index's doc_id_map
    for (const auto& [doc_id_str, doc_num_id] : forward_index.get_doc_id_map()) {
        const DocumentIndex* doc = forward_index.get_document(doc_id_str);
        if (!doc) continue;

        std::vector<std::pair<uint32_t, uint32_t>> terms;
        for (const auto& t : doc->terms) {
            terms.emplace_back(t.word_id, t.frequency);
        }

        inverted_index.add_document(doc_num_id, terms);
    }

    // Save first 1000 inverted index entries to CSV
    inverted_index.save_first_n_to_csv(indices_path + "inverted_index.csv", reverse_lex, 1000);

    // Save full inverted index to binary
    inverted_index.save_to_binary(indices_path + "inverted_index.bin", reverse_lex);

    std::cout << "\n=== Indexing Complete ===" << std::endl;
    inverted_index.print_statistics();
    return 0;
}
