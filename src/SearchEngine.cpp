#include "../include/SearchEngine.hpp"
#include <iostream>
#include <iomanip>
#include <queue>
#include <chrono>

SearchEngine::SearchEngine(LexiconBuilder* lex, 
                           ForwardIndex* fwd_idx, 
                           InvertedIndex* inv_idx,
                           TextPreprocessor* prep)
    : lexicon(lex), 
      forward_index(fwd_idx), 
      inverted_index(inv_idx),
      preprocessor(prep) {
    
    // Build reverse lexicon for word_id -> word lookups
    reverse_lexicon = lexicon->build_reverse_lexicon();
    
    // Get corpus statistics for BM25
    average_doc_length = forward_index->get_average_doc_length();
    total_documents = forward_index->get_doc_id_map().size();
    
    std::cout << "\n=== Search Engine Initialized ===" << std::endl;
    std::cout << "Total documents indexed: " << total_documents << std::endl;
    std::cout << "Average document length: " << average_doc_length << " terms" << std::endl;
    std::cout << "Vocabulary size: " << lexicon->get_size() << " unique words" << std::endl;
    std::cout << "================================\n" << std::endl;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int top_k) 
{
    auto search_start = std::chrono::high_resolution_clock::now();
    std::vector<SearchResult> results;
    
    // Step 1: Preprocess query (same as we did for documents)
    std::cout << "\n[Step 1] Processing query: \"" << query << "\"" << std::endl;
    std::vector<std::string> query_tokens = preprocessor->preprocess(query);
    
    if (query_tokens.empty()) {
        std::cout << "Warning: Query resulted in no valid tokens after preprocessing" << std::endl;
        return results;
    }
    
    std::cout << "Query tokens: ";
    for (const auto& token : query_tokens) {
        std::cout << token << " ";
    }
    std::cout << std::endl;
    
    // Step 2: Convert query tokens to word IDs and calculate term frequencies
    std::cout << "\n[Step 2] Converting tokens to word IDs..." << std::endl;
    std::vector<uint32_t> query_word_ids;
    std::unordered_map<uint32_t, uint32_t> query_term_freq;
    
    for (const auto& token : query_tokens) {
        uint32_t word_id = lexicon->get_word_id(token);
        if (word_id != UINT32_MAX) {
            query_word_ids.push_back(word_id);
            query_term_freq[word_id]++;
        } else {
            std::cout << "  Warning: Token '" << token << "' not found in lexicon" << std::endl;
        }
    }
    
    if (query_word_ids.empty()) {
        std::cout << "Error: No query terms found in the index" << std::endl;
        return results;
    }
    
    std::cout << "Valid query terms: " << query_word_ids.size() << std::endl;
    
    // Step 3: Load barrels and retrieve candidate documents
    // ✅ OPTIMIZATION: Batch load all needed barrels ONCE
    auto barrel_start = std::chrono::high_resolution_clock::now();
    std::cout << "\n[Step 3] Loading barrels..." << std::endl;
    
    std::unordered_set<int> barrels_needed;
    for (uint32_t word_id : query_word_ids) {
        int barrel_idx = inverted_index->find_barrel_index(word_id);
        if (barrel_idx != -1) {
            barrels_needed.insert(barrel_idx);
        }
    }
    
    std::cout << "  Barrels needed: " << barrels_needed.size() << std::endl;
    
    // Load each unique barrel only once
    for (int barrel_idx : barrels_needed) {
        inverted_index->load_barrel_by_index(barrel_idx, reverse_lexicon);
    }
    
    auto barrel_end = std::chrono::high_resolution_clock::now();
    auto barrel_time = std::chrono::duration_cast<std::chrono::milliseconds>(barrel_end - barrel_start);
    std::cout << "  [Barrel loading time: " << barrel_time.count() << " ms]" << std::endl;
    
    // Retrieve candidate documents
    std::cout << "\n[Step 4] Retrieving candidate documents..." << std::endl;
    std::unordered_set<uint32_t> candidate_docs; // Set of document internal IDs
    
    for (uint32_t word_id : query_word_ids) {
        // Get posting list for this word (already loaded)
        const auto* postings = inverted_index->get_terms(word_id);
        
        if (postings) {
            std::cout << "  Word ID " << word_id << " ('" 
                     << reverse_lexicon[word_id] << "'): found in " 
                     << postings->size() << " documents" << std::endl;
            
            for (const auto& [doc_internal_id, freq] : *postings) {
                candidate_docs.insert(doc_internal_id);
            }
        }
    }
    
    std::cout << "Total candidate documents: " << candidate_docs.size() << std::endl;
    
    if (candidate_docs.empty()) {
        std::cout << "No documents found matching the query" << std::endl;
        return results;
    }
    
    // Step 5: Calculate BM25 scores for all candidates
    auto scoring_start = std::chrono::high_resolution_clock::now();
    std::cout << "\n[Step 5] Calculating BM25 scores..." << std::endl;
    
    // Get doc_id_map to convert internal IDs to doc_id strings
    auto doc_id_map = forward_index->get_doc_id_map();
    
    // Create reverse mapping: internal_id -> doc_id_string
    std::unordered_map<uint32_t, std::string> reverse_doc_map;
    for (const auto& [doc_id_str, internal_id] : doc_id_map) {
        reverse_doc_map[internal_id] = doc_id_str;
    }
    
    // ✅ OPTIMIZATION: Pre-compute IDF values once per query term
    std::unordered_map<uint32_t, double> idf_cache;
    std::unordered_map<uint32_t, uint32_t> doc_freq_cache;
    
    for (uint32_t word_id : query_word_ids) {
        const auto* postings = inverted_index->get_terms(word_id);
        uint32_t doc_frequency = postings ? postings->size() : 0;
        if (doc_frequency > 0) {
            doc_freq_cache[word_id] = doc_frequency;
            idf_cache[word_id] = calculate_idf(word_id, doc_frequency);
        }
    }
    
    for (uint32_t doc_internal_id : candidate_docs) {
        // Get document ID string
        auto it = reverse_doc_map.find(doc_internal_id);
        if (it == reverse_doc_map.end()) {
            continue; // Skip if mapping not found
        }
        
        std::string doc_id_str = it->second;
        
        // ✅ OPTIMIZATION: Use optimized BM25 calculation with cached values
        double score = calculate_bm25_optimized(doc_id_str, query_word_ids, 
                                                query_term_freq, idf_cache);
        
        // Get document details
        const DocumentIndex* doc = forward_index->get_document(doc_id_str);
        if (!doc) continue;
        
        // Create search result
        SearchResult result;
        result.doc_id = doc_id_str;
        result.title = doc->title;
        result.abstract = doc->abstract_text;
        result.score = score;
        result.doc_internal_id = doc_internal_id;
        
        // Store matched terms (optimized: get terms once)
        const auto* doc_terms = forward_index->get_document_terms(doc_id_str);
        if (doc_terms) {
            // Build quick lookup map
            std::unordered_map<uint32_t, uint32_t> term_freq_map;
            for (const auto& term : *doc_terms) {
                term_freq_map[term.word_id] = term.frequency;
            }
            
            for (uint32_t word_id : query_word_ids) {
                auto tf_it = term_freq_map.find(word_id);
                if (tf_it != term_freq_map.end()) {
                    result.matched_terms[reverse_lexicon[word_id]] = tf_it->second;
                }
            }
        }
        
        results.push_back(result);
    }
    
    auto scoring_end = std::chrono::high_resolution_clock::now();
    auto scoring_time = std::chrono::duration_cast<std::chrono::milliseconds>(scoring_end - scoring_start);
    std::cout << "  [Scoring time: " << scoring_time.count() << " ms]" << std::endl;
    
    // Step 6: Sort by score and return top K
    std::cout << "\n[Step 6] Sorting and selecting top " << top_k << " results..." << std::endl;
    std::sort(results.begin(), results.end(), 
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });
    
    // Keep only top K
    if (results.size() > static_cast<size_t>(top_k)) {
        results.resize(top_k);
    }
    
    std::cout << "Returning " << results.size() << " results" << std::endl;
    
    auto search_end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);
    std::cout << "\n[*** TOTAL SEARCH TIME: " << total_time.count() << " ms ***]\n" << std::endl;
    
    return results;
}

// ✅ NEW: Optimized BM25 calculation with pre-computed IDF values
double SearchEngine::calculate_bm25_optimized(
    const std::string& doc_id_str,
    const std::vector<uint32_t>& query_word_ids,
    const std::unordered_map<uint32_t, uint32_t>& query_term_freq,
    const std::unordered_map<uint32_t, double>& idf_cache) {
    
    double score = 0.0;
    
    // Get document length
    uint32_t doc_length = forward_index->get_document_length(doc_id_str);
    
    if (doc_length == 0) {
        return 0.0;
    }
    
    // Get document terms once
    const auto* doc_terms = forward_index->get_document_terms(doc_id_str);
    if (!doc_terms) {
        return 0.0;
    }
    
    // Build term frequency lookup map (O(n) once, then O(1) lookups)
    std::unordered_map<uint32_t, uint32_t> term_freq_map;
    for (const auto& term : *doc_terms) {
        term_freq_map[term.word_id] = term.frequency;
    }
    
    // Calculate BM25 for each query term
    for (uint32_t word_id : query_word_ids) {
        // Get term frequency in document (O(1) lookup)
        auto tf_it = term_freq_map.find(word_id);
        if (tf_it == term_freq_map.end()) {
            continue; // Term not in this document
        }
        
        uint32_t term_freq = tf_it->second;
        
        // Get pre-computed IDF (O(1) lookup)
        auto idf_it = idf_cache.find(word_id);
        if (idf_it == idf_cache.end()) {
            continue;
        }
        
        double idf = idf_it->second;
        
        // Calculate BM25 component for this term
        double numerator = term_freq * (k1 + 1.0);
        double denominator = term_freq + k1 * (1.0 - b + b * (doc_length / average_doc_length));
        
        double term_score = idf * (numerator / denominator);
        
        // Weight by query term frequency (if term appears multiple times in query)
        auto qtf_it = query_term_freq.find(word_id);
        if (qtf_it != query_term_freq.end() && qtf_it->second > 1) {
            term_score *= qtf_it->second;
        }
        
        score += term_score;
    }
    
    return score;
}

// Keep the old calculate_bm25 for backward compatibility (if needed elsewhere)
double SearchEngine::calculate_bm25(uint32_t doc_internal_id,
                                   const std::string& doc_id_str,
                                   const std::vector<uint32_t>& query_word_ids,
                                   const std::unordered_map<uint32_t, uint32_t>& query_term_freq) {
    double score = 0.0;
    
    // Get document length
    uint32_t doc_length = forward_index->get_document_length(doc_id_str);
    
    if (doc_length == 0) {
        return 0.0;
    }
    
    // Calculate BM25 for each query term
    for (uint32_t word_id : query_word_ids) {
        // Get term frequency in document
        uint32_t term_freq = forward_index->get_term_frequency(doc_id_str, word_id);
        
        if (term_freq == 0) {
            continue; // Term not in this document
        }
        
        // Get document frequency (number of documents containing this term)
        const auto* postings = inverted_index->get_terms(word_id);
        uint32_t doc_frequency = postings ? postings->size() : 0;
        
        if (doc_frequency == 0) {
            continue;
        }
        
        // Calculate IDF
        double idf = calculate_idf(word_id, doc_frequency);
        
        // Calculate BM25 component for this term
        double numerator = term_freq * (k1 + 1.0);
        double denominator = term_freq + k1 * (1.0 - b + b * (doc_length / average_doc_length));
        
        double term_score = idf * (numerator / denominator);
        
        // Weight by query term frequency (if term appears multiple times in query)
        auto qtf_it = query_term_freq.find(word_id);
        if (qtf_it != query_term_freq.end() && qtf_it->second > 1) {
            term_score *= qtf_it->second;
        }
        
        score += term_score;
    }
    
    return score;
}

double SearchEngine::calculate_idf(uint32_t word_id, uint32_t doc_frequency) {
    // IDF = log((N - df + 0.5) / (df + 0.5) + 1)
    // Where N = total documents, df = document frequency
    
    if (doc_frequency == 0) {
        return 0.0;
    }
    
    double numerator = total_documents - doc_frequency + 0.5;
    double denominator = doc_frequency + 0.5;
    
    return std::log((numerator / denominator) + 1.0);
}

const DocumentIndex* SearchEngine::get_document(const std::string& doc_id) const {
    return forward_index->get_document(doc_id);
}

void SearchEngine::print_results(const std::vector<SearchResult>& results) const {
    if (results.empty()) {
        std::cout << "\nNo results found." << std::endl;
        return;
    }
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "SEARCH RESULTS (" << results.size() << " documents)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    for (size_t i = 0; i < results.size(); i++) {
        const auto& result = results[i];
        
        std::cout << "\n[" << (i + 1) << "] Score: " << std::fixed 
                  << std::setprecision(4) << result.score << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        std::cout << "Title: " << result.title << std::endl;
        std::cout << "Doc ID: " << result.doc_id << std::endl;
        
        // Print matched terms
        std::cout << "Matched terms: ";
        for (const auto& [term, freq] : result.matched_terms) {
            std::cout << term << "(" << freq << ") ";
        }
        std::cout << std::endl;
        
        // Print abstract (truncated to 200 chars)
        std::string abstract = result.abstract;
        if (abstract.length() > 200) {
            abstract = abstract.substr(0, 200) + "...";
        }
        std::cout << "Abstract: " << abstract << std::endl;
    }
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
}