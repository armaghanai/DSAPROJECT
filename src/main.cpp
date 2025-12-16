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
#include <vector>
#include <sstream>
#include "../include/nlohmann_json.hpp"

using json = nlohmann::json;

// ---------------------- Timing Utility ----------------------
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

// ---------------------- API Mode Function ----------------------
int run_api(int argc, char** argv,
            const std::string& indices_path,
            const std::string& embedding_file,
            const std::string& autocomplete_cache)
{
    std::string mode = argv[1];   // "search" | "suggest"
    std::string query = argv[2];

    // Minimal engine initialization
    TextPreprocessor preprocessor;
    LexiconBuilder lexicon;
    lexicon.load_from_csv(indices_path + "lexicon.csv");

    ForwardIndex forward_index;
    forward_index.load_from_binary(indices_path + "forward_index.bin");

    InvertedIndex inverted_index;
    auto reverse_lex = lexicon.build_reverse_lexicon();
    inverted_index.load_from_binary(indices_path + "inverted_index.bin", reverse_lex);

    CachedSearchEngine bm25(&lexicon, &forward_index, &inverted_index, &preprocessor);

    AutoComplete autocomplete(10, false, 2);
    autocomplete.load_from_binary(autocomplete_cache);

    auto start = std::chrono::high_resolution_clock::now();
    json response;

    if (mode == "suggest") {
        auto suggestions = autocomplete.get_suggestions(query, 10);
        for (auto& s : suggestions)
            response["data"].push_back(s.word);
    }

    if (mode == "search") {
        auto results = bm25.search(query, 10);
        for (auto& r : results)
            response["results"].push_back({
                {"docId", r.doc_id},
                {"score", r.score}
            });
    }

    auto end = std::chrono::high_resolution_clock::now();
    response["time_ms"] =
        std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << response.dump() << std::endl;
    return 0;
}

// ---------------------- Main Function ----------------------
int main(int argc, char** argv)
{
    std::string indices_path = "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    std::string embedding_file = "D:\\3RD SEMESTER\\Data Structures and Algorithms\\dsa search engine\\DSA-Searh-Engine\\embeddings\\glove.6B.100d.bin";
    std::string autocomplete_cache = indices_path + "autocomplete.bin";

    // ------------------ API Mode ------------------
    if (argc >= 3) {
        return run_api(argc, argv, indices_path, embedding_file, autocomplete_cache);
    }

    // ------------------ Interactive Mode ------------------
    std::cout << "=== COVID-19 Research Paper Search Engine ===\n"
              << "With BM25 Keyword + Semantic Search + AutoComplete\n"
              << "==========================================\n" << std::endl;

    Timer t1("Step 1: Loading Lexicon");
    LexiconBuilder lexicon;
    if (!lexicon.load_from_csv(indices_path + "lexicon.csv")) {
        std::cerr << "Failed to load lexicon!" << std::endl;
        return 1;
    }
    auto reverse_lex = lexicon.build_reverse_lexicon();

    Timer t2("Step 2: Loading Forward Index");
    ForwardIndex forward_index;
    if (!forward_index.load_from_binary(indices_path + "forward_index.bin")) return 1;

    Timer t3("Step 3: Loading Inverted Index");
    InvertedIndex inverted_index;
    if (!inverted_index.load_from_binary(indices_path + "inverted_index.bin", reverse_lex)) return 1;

    Timer t4("Step 4: Setting up Barrels");
    if (!std::filesystem::exists(barrel_path)) inverted_index.create_barrels(barrel_path, reverse_lex, 10);
    inverted_index.load_barrel_metadata(barrel_path);

    TextPreprocessor preprocessor;
    CachedSearchEngine bm25_engine(&lexicon, &forward_index, &inverted_index, &preprocessor);

    Timer t5("Step 5: Loading Word Embeddings");
    WordEmbeddingsEngine embeddings_engine;
    if (std::filesystem::exists(embedding_file)) embeddings_engine.load_embeddings_binary(embedding_file);

    ParallelSemanticSearch semantic_engine(&embeddings_engine, &bm25_engine);
    semantic_engine.set_weights(0.6f, 0.4f);

    AutoComplete autocomplete(10, false, 2);
    if (std::filesystem::exists(autocomplete_cache)) autocomplete.load_from_binary(autocomplete_cache);
    else autocomplete.initialize_from_lexicon(&lexicon);

    // ------------------ Interactive Loop ------------------
    std::string query;
    int search_mode = 0;
    while (true) {
        std::cout << "\n[1] BM25 Search  [2] Semantic Search  [3] Hybrid Search  [4] AutoComplete  [0] Exit\n";
        std::cout << "Select (0-4): ";
        if (!(std::cin >> search_mode)) break;
        std::cin.ignore();
        if (search_mode == 0) break;

        std::cout << "Enter query: ";
        std::getline(std::cin, query);
        if (query.empty()) continue;

        auto prep_start = std::chrono::high_resolution_clock::now();
        auto tokens = preprocessor.preprocess(query);
        auto prep_end = std::chrono::high_resolution_clock::now();
        std::cout << "Query preprocessing: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(prep_end - prep_start).count()
                  << " ms\n";

        auto search_start = std::chrono::high_resolution_clock::now();
        if (search_mode == 1) {
            auto results = bm25_engine.search(query, 10);
            bm25_engine.print_results(results);
        } else if (search_mode == 2) {
            auto results = semantic_engine.semantic_search(query, tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
        } else if (search_mode == 3) {
            auto results = semantic_engine.hybrid_search(query, tokens, &forward_index, 10);
            semantic_engine.print_semantic_results(results);
        } else if (search_mode == 4) {
            auto suggestions = autocomplete.get_suggestions(query, 10);
            for (auto& s : suggestions) std::cout << s.word << " ";
            std::cout << std::endl;
        }
        auto search_end = std::chrono::high_resolution_clock::now();
        std::cout << "Total search time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start).count()
                  << " ms\n";
    }

    std::cout << "Thank you for using the search engine!\n";
    return 0;
}
