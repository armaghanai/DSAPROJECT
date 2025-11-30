#include "../include/metadataparser.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include <iostream>
#include <iomanip>
#include <string>

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
    std::cout << "Total papers parsed: " << parser.papers.size() << std::endl;

    // =================== Step 3: Preprocessor ===================
    TextPreprocessor preprocessor;

    // =================== Step 4: Build Lexicon ===================
    std::cout << "\n=== Building Lexicon ===" << std::endl;
    LexiconBuilder lexicon;
    for (const auto& paper : parser.papers) {
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
    for (const auto& paper : parser.papers) {
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
    std::string forward_csv = indices_path + "forward_index.csv";
    forward_index.save_first_n_to_csv(forward_csv, 1000);

    // =================== Step 7: Save full forward index to binary ===================
    std::string forward_bin = indices_path + "forward_index.bin";
    forward_index.save_to_binary(forward_bin);

    // =================== Step 8: Load forward index from binary ===================
    std::cout << "\n=== Testing Forward Index Loading ===" << std::endl;
    ForwardIndex test_index;
    if (test_index.load_from_binary(forward_bin)) {
        std::cout << "Forward index loaded successfully!" << std::endl;
        test_index.print_statistics();

        // =================== Step 9: Sample document query ===================
        size_t sample_index = 800; // pick a sample document
        if (sample_index < test_index.get_total_documents()) {
            auto it = test_index.get_doc_id_map().begin();
            std::advance(it, sample_index);
            const std::string& doc_id = it->first;

            const DocumentIndex* doc = test_index.get_document(doc_id);
            if (doc) {
                std::cout << "\n=== Sample Document Query ===" << std::endl;
                std::cout << "Querying document: " << doc_id << std::endl;
                std::cout << "Title: " << doc->title << std::endl;
                std::cout << "Abstract (first 200 chars): " 
                          << doc->abstract_text.substr(0, 200) << "..." << std::endl;
                std::cout << "Document Length: " << doc->doc_length << " terms" << std::endl;
                std::cout << "Unique Terms: " << doc->terms.size() << std::endl;

                size_t display_terms = std::min<size_t>(5, doc->terms.size());
                std::cout << "\nFirst " << display_terms << " terms in document:" << std::endl;
                for (size_t i = 0; i < display_terms; ++i) {
                    std::cout << "  Word ID: " << doc->terms[i].word_id
                              << ", Frequency: " << doc->terms[i].frequency << std::endl;
                }
            }
        } else {
            std::cout << "Sample index exceeds total documents in forward index." << std::endl;
        }
    } else {
        std::cout << "Error loading forward index!" << std::endl;
    }

    std::cout << "\n=== Indexing Complete ===" << std::endl;
    return 0;
}
