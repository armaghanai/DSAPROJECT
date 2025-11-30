
#include "../include/InvertedIndex.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>

InvertedIndex::InvertedIndex(){}

void InvertedIndex:: add_document(uint32_t doc_id, 
    const std::vector<std::pair<uint32_t,uint32_t>>& terms)
{
    for (const auto& term : terms) 
        inverted_index[term.first].push_back(std::make_pair(doc_id, term.second));

}
std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>>InvertedIndex:: get_inverted_index()const
{
    return inverted_index;
}
const std::vector<std::pair<uint32_t,uint32_t>>* InvertedIndex:: get_terms(uint32_t word_id)
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) 
        return &(it->second);
    return nullptr;
}
void InvertedIndex::save_to_csv(const std::string& file_path,
    const std::unordered_map<uint32_t, std::string>& reverse_lex)const
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
            out << word_id << "," << word << "," << posting.first << "," << posting.second << "\n";
        }
    }
    out.close();
    std::cout << "Inverted index saved to " << file_path << std::endl;
}
void InvertedIndex::save_first_n_to_csv(const std::string& file_path,
    const std::unordered_map<uint32_t, std::string>& reverse_lex,size_t num)const
{
    std::ofstream out(file_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << file_path << " for writing.\n";
        return;
    }

    size_t n=0;

    out << "word_id,word,doc_id,frequency\n";
    for (const auto& [word_id, postings] : inverted_index) 
    {
        if(n>=num)
            break;
        std::string word = reverse_lex.at(word_id);
        for (const auto& posting : postings) {
            out << word_id << "," << word << "," << posting.first << "," << posting.second << "\n";
        }
        n++;
    }
    out.close();
    std::cout << "Inverted index (with "<<num<<" words) saved to " << file_path << std::endl;
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

    // Number of word_ids
    uint32_t num_words = inverted_index.size();
    out.write(reinterpret_cast<const char*>(&num_words), sizeof(num_words));

    for (const auto& [word_id, postings] : inverted_index) {
        // Write word_id
        out.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));

        // Write word string
        const std::string& word = reverse_lex.at(word_id);
        uint32_t word_len = word.size();
        out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));
        out.write(word.c_str(), word_len);

        // Write number of postings
        uint32_t num_postings = postings.size();
        out.write(reinterpret_cast<const char*>(&num_postings), sizeof(num_postings));

        // Write each posting: numeric doc_id + frequency
        for (const auto& [doc_id, freq] : postings) {
            out.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
            out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
        }
    }

    out.close();
    std::cout << "Inverted index saved to binary (with words) at " << file_path << std::endl;
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

    // Number of word_ids
    uint32_t num_words;
    in.read(reinterpret_cast<char*>(&num_words), sizeof(num_words));

    for (uint32_t i = 0; i < num_words; ++i) {
        uint32_t word_id;
        in.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));

        // Read word string
        uint32_t word_len;
        in.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));

        std::string word(word_len, '\0');
        in.read(&word[0], word_len);

        reverse_lex[word_id] = word;

        // Read number of postings
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
        size_t total_postings = 0;      // total word occurrences across all docs
        size_t min_docs = SIZE_MAX;      // min docs a word appears in
        size_t max_docs = 0;             // max docs a word appears in
        
        for (const auto& [word_id, postings] : inverted_index) {
            size_t doc_count = postings.size();
            total_postings += doc_count;
            min_docs = std::min(min_docs, doc_count);
            max_docs = std::max(max_docs, doc_count);
        }
        
        double avg_docs_per_word = static_cast<double>(total_postings) / inverted_index.size();
        
        std::cout << "  Total word occurrences (counting repeats): " << total_postings << '\n';
        std::cout << "  Average docs per word: " << avg_docs_per_word << '\n';
        std::cout << "  Minimum docs a word appears in: " << min_docs << '\n';
        std::cout << "  Maximum docs a word appears in: " << max_docs << '\n';
    }
}



void InvertedIndex::clear()
{
    inverted_index.clear();
}
