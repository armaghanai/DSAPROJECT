#include "../include/InvertedIndex.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

InvertedIndex::InvertedIndex() : currently_loaded_barrel(-1) {}

void InvertedIndex::add_document(uint32_t doc_id, 
    const std::vector<std::pair<uint32_t,uint32_t>>& terms)
{
    for (const auto& term : terms) 
        inverted_index[term.first].push_back(std::make_pair(doc_id, term.second));
}

std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>>
InvertedIndex::get_inverted_index() const
{
    return inverted_index;
}

const std::vector<std::pair<uint32_t,uint32_t>>* 
InvertedIndex::get_terms(uint32_t word_id)
{
    // Check if word is in currently loaded data
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) {
        return &(it->second);
    }
    
    // If barrels are available, try to load the correct barrel
    if (!barrel_metadata.empty()) {
        int barrel_idx = find_barrel_index(word_id);
        if (barrel_idx != -1) {
            // Note: This requires reverse_lex, which we don't have here
            // In practice, you should call load_barrel_for_word() before get_terms()
            std::cerr << "Warning: Word " << word_id << " not loaded. "
                     << "Call load_barrel_for_word() first.\n";
        }
    }
    
    return nullptr;
}

void InvertedIndex::save_to_csv(const std::string& file_path,
    const std::unordered_map<uint32_t, std::string>& reverse_lex) const
{
    std::ofstream out(file_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for writing.\n";
        return;
    }

    out << "word_id,word,doc_id,frequency\n";
    for (const auto& [word_id, postings] : inverted_index) 
    {
        std::string word = reverse_lex.at(word_id);
        for (const auto& posting : postings) {
            out << word_id << "," << word << "," 
                << posting.first << "," << posting.second << "\n";
        }
    }
    out.close();
    std::cout << "Inverted index saved to " << file_path << std::endl;
}

void InvertedIndex::save_first_n_to_csv(const std::string& file_path,
    const std::unordered_map<uint32_t, std::string>& reverse_lex, size_t num) const
{
    std::ofstream out(file_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for writing.\n";
        return;
    }

    size_t n = 0;
    out << "word_id,word,doc_id,frequency\n";
    for (const auto& [word_id, postings] : inverted_index) 
    {
        if (n >= num) break;
        std::string word = reverse_lex.at(word_id);
        for (const auto& posting : postings) {
            out << word_id << "," << word << "," 
                << posting.first << "," << posting.second << "\n";
        }
        n++;
    }
    out.close();
    std::cout << "Inverted index (with " << num << " words) saved to " 
              << file_path << std::endl;
}

void InvertedIndex::save_to_binary(
    const std::string& file_path, 
    const std::unordered_map<uint32_t, std::string>& reverse_lex) const
{
    std::ofstream out(file_path, std::ios::binary);
    if (!out.is_open()) 
    {
        std::cerr << "Error: Cannot open " << file_path << " for writing.\n";
        return;
    }

    uint32_t num_words = inverted_index.size();
    out.write(reinterpret_cast<const char*>(&num_words), sizeof(num_words));

    for (const auto& [word_id, postings] : inverted_index) {
        out.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));

        const std::string& word = reverse_lex.at(word_id);
        uint32_t word_len = word.size();
        out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));
        out.write(word.c_str(), word_len);

        uint32_t num_postings = postings.size();
        out.write(reinterpret_cast<const char*>(&num_postings), sizeof(num_postings));

        for (const auto& [doc_id, freq] : postings) {
            out.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
            out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
        }
    }

    out.close();
    std::cout << "Inverted index saved to binary at " << file_path << std::endl;
}

bool InvertedIndex::load_from_binary(
    const std::string& file_path, 
    std::unordered_map<uint32_t, std::string>& reverse_lex)
{
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for reading.\n";
        return false;
    }

    clear();
    reverse_lex.clear();

    uint32_t num_words;
    in.read(reinterpret_cast<char*>(&num_words), sizeof(num_words));

    for (uint32_t i = 0; i < num_words; ++i) {
        uint32_t word_id;
        in.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));

        uint32_t word_len;
        in.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));

        std::string word(word_len, '\0');
        in.read(&word[0], word_len);
        reverse_lex[word_id] = word;

        uint32_t num_postings;
        in.read(reinterpret_cast<char*>(&num_postings), sizeof(num_postings));

        std::vector<std::pair<uint32_t, uint32_t>> postings;
        postings.reserve(num_postings);

        for (uint32_t j = 0; j < num_postings; ++j) {
            uint32_t doc_id, freq;
            in.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
            in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
            postings.emplace_back(doc_id, freq);
        }

        inverted_index[word_id] = std::move(postings);
    }

    in.close();
    std::cout << "Inverted index loaded from binary: " << file_path << std::endl;
    return true;
}

void InvertedIndex::print_statistics() const
{
    std::cout << "\nInverted Index Statistics:\n";    
    std::cout << "  Total unique words: " << inverted_index.size() << '\n';

    if (!inverted_index.empty()) {
        size_t total_postings = 0;
        size_t min_docs = SIZE_MAX;
        size_t max_docs = 0;
        
        for (const auto& [word_id, postings] : inverted_index) {
            size_t doc_count = postings.size();
            total_postings += doc_count;
            min_docs = std::min(min_docs, doc_count);
            max_docs = std::max(max_docs, doc_count);
        }
        
        double avg_docs_per_word = static_cast<double>(total_postings) / inverted_index.size();
        
        std::cout << "  Total word occurrences: " << total_postings << '\n';
        std::cout << "  Average docs per word: " << avg_docs_per_word << '\n';
        std::cout << "  Min docs a word appears in: " << min_docs << '\n';
        std::cout << "  Max docs a word appears in: " << max_docs << '\n';
    }
}

void InvertedIndex::clear()
{
    inverted_index.clear();
    currently_loaded_barrel = -1;
}

// ========== NEW BARREL METHODS ==========

bool InvertedIndex::create_barrels(
    const std::string& barrel_dir,
    const std::unordered_map<uint32_t, std::string>& reverse_lex,
    uint32_t num_barrels)
{
    if (inverted_index.empty()) {
        std::cerr << "Error: Cannot create barrels from empty inverted index.\n";
        return false;
    }
    
    // Create barrel directory
    if (!fs::exists(barrel_dir)) {
        fs::create_directories(barrel_dir);
    }
    
    std::cout << "\n=== Creating " << num_barrels << " Barrels ===\n";
    
    // Find min and max word_id
    uint32_t min_word_id = UINT32_MAX;
    uint32_t max_word_id = 0;
    
    for (const auto& [word_id, postings] : inverted_index) {
        min_word_id = std::min(min_word_id, word_id);
        max_word_id = std::max(max_word_id, word_id);
    }
    
    std::cout << "Word ID range: " << min_word_id << " - " << max_word_id << "\n";
    std::cout << "Total unique words: " << inverted_index.size() << "\n";
    
    // Calculate range per barrel
    uint32_t total_range = max_word_id - min_word_id + 1;
    uint32_t range_per_barrel = (total_range + num_barrels - 1) / num_barrels;
    
    std::cout << "Range per barrel: ~" << range_per_barrel << " word IDs\n\n";
    
    // Clear and prepare metadata
    barrel_metadata.clear();
    
    // Create each barrel
    for (uint32_t barrel_id = 0; barrel_id < num_barrels; ++barrel_id) {
        uint32_t start_id = min_word_id + (barrel_id * range_per_barrel);
        uint32_t end_id = start_id + range_per_barrel - 1;
        
        // Last barrel takes remaining range
        if (barrel_id == num_barrels - 1) {
            end_id = max_word_id;
        }
        
        // Filter words in this range
        std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> barrel_data;
        
        for (const auto& [word_id, postings] : inverted_index) {
            if (word_id >= start_id && word_id <= end_id) {
                barrel_data[word_id] = postings;
            }
        }
        
        if (barrel_data.empty()) {
            std::cout << "Barrel " << barrel_id << ": EMPTY (skipping)\n";
            continue;
        }
        
        // Write barrel file
        std::string barrel_filename = "inverted_barrel_" + std::to_string(barrel_id) + ".bin";
        std::string barrel_path = barrel_dir + "/" + barrel_filename;
        
        std::ofstream out(barrel_path, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot create barrel file " << barrel_path << "\n";
            return false;
        }
        
        // Write barrel header
        out.write(reinterpret_cast<const char*>(&barrel_id), sizeof(barrel_id));
        out.write(reinterpret_cast<const char*>(&start_id), sizeof(start_id));
        out.write(reinterpret_cast<const char*>(&end_id), sizeof(end_id));
        
        // Write number of words in this barrel
        uint32_t num_words = barrel_data.size();
        out.write(reinterpret_cast<const char*>(&num_words), sizeof(num_words));
        
        // Write each word's data
        for (const auto& [word_id, postings] : barrel_data) {
            out.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));
            
            const std::string& word = reverse_lex.at(word_id);
            uint32_t word_len = word.size();
            out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));
            out.write(word.c_str(), word_len);
            
            uint32_t num_postings = postings.size();
            out.write(reinterpret_cast<const char*>(&num_postings), sizeof(num_postings));
            
            for (const auto& [doc_id, freq] : postings) {
                out.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
                out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
            }
        }
        
        out.close();
        
        // Save metadata
        BarrelMetadata meta;
        meta.barrel_id = barrel_id;
        meta.start_word_id = start_id;
        meta.end_word_id = end_id;
        meta.barrel_filename = barrel_filename;
        barrel_metadata.push_back(meta);
        
        std::cout << "Barrel " << barrel_id << ": " 
                  << num_words << " words (IDs " << start_id << "-" << end_id << ") -> "
                  << barrel_filename << "\n";
    }
    
    // Save barrel metadata file
    std::string metadata_path = barrel_dir + "/barrel_metadata.bin";
    std::ofstream meta_out(metadata_path, std::ios::binary);
    if (!meta_out.is_open()) {
        std::cerr << "Error: Cannot create metadata file.\n";
        return false;
    }
    
    uint32_t num_meta = barrel_metadata.size();
    meta_out.write(reinterpret_cast<const char*>(&num_meta), sizeof(num_meta));
    
    for (const auto& meta : barrel_metadata) {
        meta_out.write(reinterpret_cast<const char*>(&meta.barrel_id), sizeof(meta.barrel_id));
        meta_out.write(reinterpret_cast<const char*>(&meta.start_word_id), sizeof(meta.start_word_id));
        meta_out.write(reinterpret_cast<const char*>(&meta.end_word_id), sizeof(meta.end_word_id));
        
        uint32_t filename_len = meta.barrel_filename.size();
        meta_out.write(reinterpret_cast<const char*>(&filename_len), sizeof(filename_len));
        meta_out.write(meta.barrel_filename.c_str(), filename_len);
    }
    
    meta_out.close();
    
    std::cout << "\nBarrel metadata saved to: " << metadata_path << "\n";
    std::cout << "=== Barrel Creation Complete ===\n\n";
    
    barrel_directory = barrel_dir;
    return true;
}

bool InvertedIndex::load_barrel_metadata(const std::string& barrel_dir)
{
    std::string metadata_path = barrel_dir + "/barrel_metadata.bin";
    
    std::ifstream in(metadata_path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open barrel metadata: " << metadata_path << "\n";
        return false;
    }
    
    barrel_metadata.clear();
    
    uint32_t num_meta;
    in.read(reinterpret_cast<char*>(&num_meta), sizeof(num_meta));
    
    for (uint32_t i = 0; i < num_meta; ++i) {
        BarrelMetadata meta;
        
        in.read(reinterpret_cast<char*>(&meta.barrel_id), sizeof(meta.barrel_id));
        in.read(reinterpret_cast<char*>(&meta.start_word_id), sizeof(meta.start_word_id));
        in.read(reinterpret_cast<char*>(&meta.end_word_id), sizeof(meta.end_word_id));
        
        uint32_t filename_len;
        in.read(reinterpret_cast<char*>(&filename_len), sizeof(filename_len));
        
        meta.barrel_filename.resize(filename_len);
        in.read(&meta.barrel_filename[0], filename_len);
        
        barrel_metadata.push_back(meta);
    }
    
    in.close();
    
    barrel_directory = barrel_dir;
    currently_loaded_barrel = -1;
    
    std::cout << "Loaded barrel metadata: " << num_meta << " barrels\n";
    return true;
}

int InvertedIndex::find_barrel_index(uint32_t word_id) const
{
    for (size_t i = 0; i < barrel_metadata.size(); ++i) {
        if (word_id >= barrel_metadata[i].start_word_id && 
            word_id <= barrel_metadata[i].end_word_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool InvertedIndex::load_barrel_for_word(
    uint32_t word_id,
    std::unordered_map<uint32_t, std::string>& reverse_lex)
{
    int barrel_idx = find_barrel_index(word_id);
    
    if (barrel_idx == -1) {
        std::cerr << "Error: Word ID " << word_id << " not in any barrel.\n";
        return false;
    }
    
    return load_barrel_by_index(barrel_idx, reverse_lex);
}

bool InvertedIndex::load_barrel_by_index(
    int barrel_idx,
    std::unordered_map<uint32_t, std::string>& reverse_lex)
{
    if (barrel_idx < 0 || barrel_idx >= static_cast<int>(barrel_metadata.size())) {
        std::cerr << "Error: Invalid barrel index " << barrel_idx << "\n";
        return false;
    }
    
    // Check if already loaded
    if (currently_loaded_barrel == barrel_idx) {
        return true;
    }
    
    const BarrelMetadata& meta = barrel_metadata[barrel_idx];
    std::string barrel_path = barrel_directory + "/" + meta.barrel_filename;
    
    std::ifstream in(barrel_path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open barrel file " << barrel_path << "\n";
        return false;
    }
    
    // Clear current data
    inverted_index.clear();
    
    // Read barrel header
    uint32_t barrel_id, start_id, end_id;
    in.read(reinterpret_cast<char*>(&barrel_id), sizeof(barrel_id));
    in.read(reinterpret_cast<char*>(&start_id), sizeof(start_id));
    in.read(reinterpret_cast<char*>(&end_id), sizeof(end_id));
    
    // Read number of words
    uint32_t num_words;
    in.read(reinterpret_cast<char*>(&num_words), sizeof(num_words));
    
    // Read each word
    for (uint32_t i = 0; i < num_words; ++i) {
        uint32_t word_id;
        in.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));
        
        uint32_t word_len;
        in.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));
        
        std::string word(word_len, '\0');
        in.read(&word[0], word_len);
        reverse_lex[word_id] = word;
        
        uint32_t num_postings;
        in.read(reinterpret_cast<char*>(&num_postings), sizeof(num_postings));
        
        std::vector<std::pair<uint32_t, uint32_t>> postings;
        postings.reserve(num_postings);
        
        for (uint32_t j = 0; j < num_postings; ++j) {
            uint32_t doc_id, freq;
            in.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
            in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
            postings.emplace_back(doc_id, freq);
        }
        
        inverted_index[word_id] = std::move(postings);
    }
    
    in.close();
    
    currently_loaded_barrel = barrel_idx;
    
    std::cout << "Loaded barrel " << barrel_idx << ": " 
              << num_words << " words (IDs " << start_id << "-" << end_id << ")\n";
    
    return true;
}

void InvertedIndex::print_barrel_info() const
{
    if (barrel_metadata.empty()) {
        std::cout << "No barrel metadata loaded.\n";
        return;
    }
    
    std::cout << "\n=== Barrel Information ===\n";
    std::cout << "Total barrels: " << barrel_metadata.size() << "\n";
    std::cout << "Barrel directory: " << barrel_directory << "\n";
    std::cout << "Currently loaded barrel: " << currently_loaded_barrel << "\n\n";
    
    for (const auto& meta : barrel_metadata) {
        std::cout << "Barrel " << meta.barrel_id << ": "
                  << "IDs " << meta.start_word_id << "-" << meta.end_word_id
                  << " (" << meta.barrel_filename << ")\n";
    }
    
    std::cout << "=========================\n\n";
}