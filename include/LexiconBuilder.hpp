#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

class LexiconBuilder
{
    private:
        std::unordered_map<std::string,std::pair<uint32_t,uint32_t>> lexicon_data;
        int next_word_id;
    public:
        LexiconBuilder();
        uint32_t add_word(const std::string& word,uint32_t count);
        bool contains(const std::string& word);
        const std::pair<uint32_t,uint32_t>* get_word_details(const std::string& word);
        uint32_t get_frequency(const std::string& word)const;
        uint32_t get_word_id(const std::string& word)const;
        size_t get_size()const;
        void save_to_csv(const std::string& csv_path);
        bool load_from_csv(const std::string& csv_path);
        void clear_lexicon();

    

   

};