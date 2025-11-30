#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

class InvertedIndex
{
    private:
        std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>> inverted_index;

    public:
        InvertedIndex();
        void add_document(uint32_t doc_id, 
            const std::vector<std::pair<uint32_t,uint32_t>>& terms);
        const std::vector<std::pair<uint32_t,uint32_t>>* get_terms(uint32_t word_id);
        void save_to_csv(const std::string& file_path, const std::unordered_map<uint32_t, std::string>& reverse_lex) const;
        void save_first_n_to_csv(const std::string& file_path,
            const std::unordered_map<uint32_t, std::string>& reverse_lex,size_t num)const;
        std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>> get_inverted_index()const;
        void save_to_binary(const std::string& file_path, const std::unordered_map<uint32_t, std::string>& reverse_lex) const;
        bool load_from_binary(const std::string& file_path, std::unordered_map<uint32_t, std::string>& reverse_lex);
        void clear();
        void print_statistics() const;



};