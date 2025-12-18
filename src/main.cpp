// main_api.cpp - Bulletproof version that silences ALL stdout except JSON
#include "../include/CachedSearchEngine.hpp"
#include "../include/SemanticSearchEngine.hpp"
#include "../include/WordEmbeddingsEngine.hpp"
#include "../include/LexiconBuilder.hpp"
#include "../include/ForwardIndex.hpp"
#include "../include/InvertedIndex.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/AutoComplete.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

// Global null output stream to redirect cout
std::ofstream null_stream;
std::streambuf* original_cout = nullptr;

// Global objects
LexiconBuilder* lexicon = nullptr;
ForwardIndex* forward_index = nullptr;
InvertedIndex* inverted_index = nullptr;
TextPreprocessor* preprocessor = nullptr;
CachedSearchEngine* bm25_engine = nullptr;
WordEmbeddingsEngine* embeddings_engine = nullptr;
SemanticSearchEngine* semantic_engine = nullptr;
AutoComplete* autocomplete = nullptr;

bool initialized = false;

// Silence ALL stdout output
void silence_stdout() {
#ifdef _WIN32
    null_stream.open("NUL");
#else
    null_stream.open("/dev/null");
#endif
    original_cout = std::cout.rdbuf();
    std::cout.rdbuf(null_stream.rdbuf());
}

// Restore stdout for JSON output
void restore_stdout() {
    if (original_cout) {
        std::cout.rdbuf(original_cout);
    }
}

// Initialize all components
bool initialize() {
    if (initialized) return true;
    
    // SILENCE EVERYTHING
    silence_stdout();
    
    std::string indices_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\indices\\";
    std::string barrel_path = indices_path + "inverted_index_barrels";
    std::string binary_embedding_file = 
        "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\embeddings\\glove.6B.100d.bin";
    std::string autocomplete_cache = indices_path + "autocomplete.bin";
    
    try {
        // Load Lexicon
        lexicon = new LexiconBuilder();
        if (!lexicon->load_from_csv(indices_path + "lexicon.csv")) {
            return false;
        }
        auto reverse_lex = lexicon->build_reverse_lexicon();
        
        // Load Forward Index
        forward_index = new ForwardIndex();
        if (!forward_index->load_from_binary(indices_path + "forward_index.bin")) {
            return false;
        }
        
        // Load Inverted Index
        inverted_index = new InvertedIndex();
        if (!inverted_index->load_from_binary(indices_path + "inverted_index.bin", reverse_lex)) {
            return false;
        }
        
        // Load Barrels
        if (!inverted_index->load_barrel_metadata(barrel_path)) {
            return false;
        }
        
        // Initialize preprocessor
        preprocessor = new TextPreprocessor();
        
        // Initialize BM25 Engine
        bm25_engine = new CachedSearchEngine(lexicon, forward_index, inverted_index, preprocessor);
        
        // Load Word Embeddings
        embeddings_engine = new WordEmbeddingsEngine();
        if (fs::exists(binary_embedding_file)) {
            embeddings_engine->load_embeddings_binary(binary_embedding_file);
        }
        
        // Initialize Semantic Engine
        semantic_engine = new SemanticSearchEngine(embeddings_engine, bm25_engine);
        semantic_engine->set_weights(0.6f, 0.4f);
        
        // Load AutoComplete
        autocomplete = new AutoComplete(10, false, 2);
        if (fs::exists(autocomplete_cache)) {
            autocomplete->load_from_binary(autocomplete_cache);
        } else {
            autocomplete->initialize_from_lexicon(lexicon);
            autocomplete->save_to_binary(autocomplete_cache);
        }
        
        initialized = true;
        return true;
        
    } catch (...) {
        return false;
    }
}

// Escape string for JSON
std::string escape_json(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            default:
                if (c < 32) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

// Output BM25 result as JSON
void output_bm25_result(const SearchResult& result, bool is_last) {
    std::cout << "    {\n";
    std::cout << "      \"doc_id\": \"" << escape_json(result.doc_id) << "\",\n";
    std::cout << "      \"title\": \"" << escape_json(result.title) << "\",\n";
    std::cout << "      \"abstract\": \"" << escape_json(result.abstract) << "\",\n";
    std::cout << "      \"score\": " << result.score << ",\n";
    std::cout << "      \"matched_terms\": {";
    
    int count = 0;
    for (const auto& [term, freq] : result.matched_terms) {
        if (count > 0) std::cout << ", ";
        std::cout << "\"" << escape_json(term) << "\": " << freq;
        count++;
    }
    
    std::cout << "}\n";
    std::cout << "    }";
    if (!is_last) std::cout << ",";
    std::cout << "\n";
}

// Output semantic result as JSON
void output_semantic_result(const SemanticSearchResult& result, bool is_last) {
    std::cout << "    {\n";
    std::cout << "      \"doc_id\": \"" << escape_json(result.doc_id) << "\",\n";
    std::cout << "      \"title\": \"" << escape_json(result.title) << "\",\n";
    std::cout << "      \"abstract\": \"" << escape_json(result.abstract) << "\",\n";
    std::cout << "      \"semantic_score\": " << result.semantic_score << ",\n";
    std::cout << "      \"bm25_score\": " << result.bm25_score << ",\n";
    std::cout << "      \"combined_score\": " << result.combined_score << ",\n";
    std::cout << "      \"matched_terms\": {";
    
    if (!result.matched_terms.empty()) {
        int count = 0;
        for (const auto& term : result.matched_terms) {
            if (count > 0) std::cout << ", ";
            std::cout << "\"" << escape_json(term) << "\": 1";
            count++;
        }
    }
    
    std::cout << "}\n";
    std::cout << "    }";
    if (!is_last) std::cout << ",";
    std::cout << "\n";
}

// Handle search
void handle_search(const std::string& query, const std::string& mode, int top_k) {
    if (!initialize()) {
        restore_stdout();
        std::cout << "{\"error\": \"Initialization failed\"}" << std::endl;
        return;
    }
    
    // Keep stdout silenced during preprocessing
    auto query_tokens = preprocessor->preprocess(query);
    
    // Restore for JSON output
    restore_stdout();
    
    std::cout << "{\n";
    std::cout << "  \"query\": \"" << escape_json(query) << "\",\n";
    std::cout << "  \"mode\": \"" << mode << "\",\n";
    std::cout << "  \"results\": [\n";
    
    // Silence again for search
    silence_stdout();
    
    if (mode == "bm25") {
        auto results = bm25_engine->search(query, top_k);
        restore_stdout();
        
        for (size_t i = 0; i < results.size(); i++) {
            output_bm25_result(results[i], i == results.size() - 1);
        }
        std::cout << "  ],\n";
        std::cout << "  \"total_results\": " << results.size() << "\n";
        
    } else if (mode == "semantic") {
        auto results = semantic_engine->semantic_search(query, query_tokens, forward_index, top_k);
        restore_stdout();
        
        for (size_t i = 0; i < results.size(); i++) {
            output_semantic_result(results[i], i == results.size() - 1);
        }
        std::cout << "  ],\n";
        std::cout << "  \"total_results\": " << results.size() << "\n";
        
    } else if (mode == "hybrid") {
        auto results = semantic_engine->hybrid_search(query, query_tokens, forward_index, top_k);
        restore_stdout();
        
        for (size_t i = 0; i < results.size(); i++) {
            output_semantic_result(results[i], i == results.size() - 1);
        }
        std::cout << "  ],\n";
        std::cout << "  \"total_results\": " << results.size() << "\n";
    }
    
    std::cout << "}" << std::endl;
}

// Handle autocomplete
void handle_autocomplete(const std::string& prefix, int limit) {
    if (!initialize()) {
        restore_stdout();
        std::cout << "{\"suggestions\": []}" << std::endl;
        return;
    }
    
    auto suggestions = autocomplete->get_suggestions(prefix, limit);
    restore_stdout();
    
    std::cout << "{\n";
    std::cout << "  \"prefix\": \"" << escape_json(prefix) << "\",\n";
    std::cout << "  \"suggestions\": [\n";
    
    for (size_t i = 0; i < suggestions.size(); i++) {
        std::cout << "    {\n";
        std::cout << "      \"word\": \"" << escape_json(suggestions[i].word) << "\",\n";
        std::cout << "      \"frequency\": " << suggestions[i].frequency << "\n";
        std::cout << "    }";
        if (i < suggestions.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    
    std::cout << "  ]\n";
    std::cout << "}" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  --search <query> --mode <bm25|semantic|hybrid> --topk <N>" << std::endl;
        std::cerr << "  --autocomplete <prefix> --limit <N>" << std::endl;
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "--search") {
        if (argc < 3) {
            std::cout << "{\"error\": \"Missing query\"}" << std::endl;
            return 1;
        }
        
        std::string query = argv[2];
        std::string mode = "hybrid";
        int top_k = 10;
        
        for (int i = 3; i < argc - 1; i++) {
            if (std::strcmp(argv[i], "--mode") == 0) {
                mode = argv[i + 1];
            } else if (std::strcmp(argv[i], "--topk") == 0) {
                top_k = std::stoi(argv[i + 1]);
            }
        }
        
        handle_search(query, mode, top_k);
        
    } else if (command == "--autocomplete") {
        if (argc < 3) {
            std::cout << "{\"suggestions\": []}" << std::endl;
            return 1;
        }
        
        std::string prefix = argv[2];
        int limit = 10;
        
        if (argc >= 5 && std::strcmp(argv[3], "--limit") == 0) {
            limit = std::stoi(argv[4]);
        }
        
        handle_autocomplete(prefix, limit);
        
    } else {
        std::cout << "{\"error\": \"Unknown command\"}" << std::endl;
        return 1;
    }
    
    return 0;
}