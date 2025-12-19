#include <iostream>
#include <string>
#include <chrono>
#include <nlohmann_json.hpp>
#include "LexiconBuilder.hpp"
#include "ForwardIndex.hpp"
#include "InvertedIndex.hpp"
#include "TextPreprocessor.hpp"
#include "CachedSearchEngine.hpp"
#include "AutoComplete.hpp"
#include "QueryLemmatizer.hpp"

using json = nlohmann::json;

// GLOBAL STATE - Initialize ONCE, reuse forever
struct SearchState {
    LexiconBuilder lexicon;
    ForwardIndex forward_index;
    InvertedIndex inverted_index;
    TextPreprocessor preprocessor;
    Lemmatizer lemmatizer;  // ✅ Full lemmatizer - initialized ONCE
    CachedSearchEngine* search_engine = nullptr;
    AutoComplete* autocomplete = nullptr;
    bool initialized = false;
};

static SearchState g_state;

// Initialize search engine ONCE
bool initialize_search_engine(const std::string& indices_path) {
    if (g_state.initialized) {
        return true; // Already initialized
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "\n=== Initializing Search Engine (ONE TIME) ===" << std::endl;
    
    // Load lexicon
    std::cout << "Loading lexicon..." << std::endl;
    if (!g_state.lexicon.load_from_csv(indices_path + "lexicon.csv")) {
        std::cerr << "Failed to load lexicon" << std::endl;
        return false;
    }
    
    // Load forward index
    std::cout << "Loading forward index..." << std::endl;
    if (!g_state.forward_index.load_from_binary(indices_path + "forward_index.bin")) {
        std::cerr << "Failed to load forward index" << std::endl;
        return false;
    }
    
  // Load barrel metadata
        std::cout << "Loading inverted index metadata..." << std::endl;
        if (!g_state.inverted_index.load_barrel_metadata(indices_path + "barrels")) {
            std::cerr << "Failed to load barrel metadata" << std::endl;
            return false;
        }

     


    // Create search engine (with shared lemmatizer)
    std::cout << "Creating search engine..." << std::endl;
    g_state.search_engine = new CachedSearchEngine(
        &g_state.lexicon,
        &g_state.forward_index,
        &g_state.inverted_index,
        &g_state.preprocessor,
        &g_state.lemmatizer  // ✅ Pass shared lemmatizer
    );
    
    // Initialize autocomplete
    std::cout << "Initializing autocomplete..." << std::endl;
    g_state.autocomplete = new AutoComplete(10, false, 2);
    g_state.autocomplete->initialize_from_lexicon(&g_state.lexicon);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\n✓ Search Engine Ready!" << std::endl;
    std::cout << "  Initialization time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Lexicon size: " << g_state.lexicon.get_size() << " words" << std::endl;
    std::cout << "  Documents: " << g_state.forward_index.get_index_size() << std::endl;
    std::cout << "=========================================\n" << std::endl;
    
    g_state.initialized = true;
    return true;
}

// Handle search request
// Handle search request
// Handle search request
void handle_search(const std::string& query, const std::string& mode, int top_k, int request_id) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform search (lemmatization happens inside, using SHARED lemmatizer - FAST!)
    auto results = g_state.search_engine->search(query, top_k);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Build JSON response
    json response;
    response["request_id"] = request_id;  // ✅ Include request_id for tracking
    response["query"] = query;
    response["mode"] = mode;
    response["search_time_ms"] = duration.count();
    response["total_results"] = results.size();
    
    json results_array = json::array();
    for (const auto& result : results) {
        json r;
        r["doc_id"] = result.doc_id;
        r["title"] = result.title;
        r["abstract"] = result.abstract;
        r["score"] = result.score;
        
        json matched = json::object();
        for (const auto& [term, freq] : result.matched_terms) {
            matched[term] = freq;
        }
        r["matched_terms"] = matched;
        
        results_array.push_back(r);
    }
    
    response["results"] = results_array;
    
    // Output JSON to stdout (Node.js will read this)
    std::cout << response.dump() << std::endl;
}

// Handle autocomplete request
void handle_autocomplete(const std::string& prefix, int limit, int request_id) {
    auto suggestions = g_state.autocomplete->get_suggestions(prefix, limit);
    
    json response;
    response["request_id"] = request_id;  // ✅ Include request_id
    
    json sugg_array = json::array();
    
    for (const auto& sugg : suggestions) {
        json s;
        s["word"] = sugg.word;
        s["frequency"] = sugg.frequency;
        sugg_array.push_back(s);
    }
    
    response["suggestions"] = sugg_array;
    
    std::cout << response.dump() << std::endl;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string indices_path = "D:\\THird Semester\\DSA\\dsaspp\\DSAPROJECT\\indices\\";

    // ✅ Initialize ONCE (takes a few seconds)
    std::cerr << "[C++ Init] Starting initialization..." << std::endl;
    if (!initialize_search_engine(indices_path)) {
        std::cerr << "Failed to initialize search engine" << std::endl;
        return 1;
    }

    // ✅ Signal that we're ready to accept requests
    std::cout << R"({"status":"ready"})" << std::endl;
    std::cout.flush();
    std::cerr << "[C++ Init] Ready to accept requests via stdin" << std::endl;

    // 🔁 Persistent request loop (processes multiple queries WITHOUT reinitializing!)
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        try {
            json request = json::parse(line);
            std::string type = request.value("type", "");
            int request_id = request.value("request_id", 0);

            if (type == "search") {
                std::string query = request.value("query", "");
                std::string mode = request.value("mode", "default");
                int top_k = request.value("top_k", 10);

                std::cerr << "[C++ Search] Query: \"" << query << "\" (req_id=" << request_id << ")" << std::endl;
                handle_search(query, mode, top_k, request_id);
            }
            else if (type == "autocomplete") {
                std::string prefix = request.value("prefix", "");
                int limit = request.value("limit", 5);

                handle_autocomplete(prefix, limit, request_id);
            }
            else if (type == "exit") {
                std::cerr << "[C++ Exit] Graceful shutdown" << std::endl;
                break;
            }
            else {
                json err;
                err["request_id"] = request_id;
                err["error"] = "Unknown request type";
                std::cout << err.dump() << std::endl;
            }
            
            std::cout.flush();
        }
        catch (const std::exception& e) {
            json err;
            err["error"] = "Invalid JSON request";
            err["message"] = e.what();
            std::cout << err.dump() << std::endl;
            std::cout.flush();
        }
    }

    // Cleanup
    delete g_state.search_engine;
    delete g_state.autocomplete;

    return 0;
}