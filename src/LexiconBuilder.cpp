#include "../include/LexiconBuilder.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <vector>

LexiconBuilder::LexiconBuilder():next_word_id(0){}   // constructor initializes next word id to 0

uint32_t LexiconBuilder::add_word(const std::string &word, uint32_t count)
{
    // check if the word already exists in the lexicon
    auto target = lexicon_data.find(word);

    if(target != lexicon_data.end())
    {
        // if found, update its frequency and return its already assigned id
        target->second.second += count;
        return target->second.first;
    }

    // assign a new word id since the word is not present
    uint32_t word_id = next_word_id++;
    lexicon_data[word] = { word_id, count };

    // return the new id
    return word_id;
}

bool LexiconBuilder::contains(const std::string& word)
{
    // simply check if a word exists in the map
    return lexicon_data.find(word) != lexicon_data.end();
}

const std::pair<uint32_t,uint32_t>* LexiconBuilder::get_word_details(const std::string& word)
{
    // finds the word and returns a pointer to its (word_id, freq) pair
    auto a = lexicon_data.find(word);
    if(a != lexicon_data.end())
        return &a->second;

    // return nullptr if the word is not found
    return nullptr;
}

uint32_t LexiconBuilder::get_word_id(const std::string& word)const
{
    // return the stored id if the word exists
    auto a = lexicon_data.find(word);
    if(a != lexicon_data.end())
        return a->second.first;

    // return max uint32_t to indicate "not found"
    return UINT32_MAX;
}

uint32_t LexiconBuilder::get_frequency(const std::string& word)const
{
    // return the frequency of the word if found
    auto a = lexicon_data.find(word);
    if(a != lexicon_data.end())
        return a->second.second;

    // again return UINT32_MAX for missing word
    return UINT32_MAX;
}

size_t LexiconBuilder::get_size()const
{
    // returns how many words are currently stored in the lexicon
    return lexicon_data.size();
}

void LexiconBuilder::save_to_csv(const std::string& csv_path)
{
    std::ofstream out(csv_path);

    if(!out.is_open())
    {
        std::cerr<<"Error! cannot open file: "<<csv_path<<" for writing\n";
        return;
    }

    // unordered_map is not sorted, so we copy data into a vector first
    std::vector<std::pair<std::string, std::pair<uint32_t,uint32_t>>> sorted(
    lexicon_data.begin(), lexicon_data.end());

    // sort by frequency 
    std::sort(sorted.begin(), sorted.end(),
          [](const auto& a, const auto& b){ return a.second.second > b.second.second; });

    // write CSV header
    out << "word,word_id,frequency\n";

    // write every entry in the sorted order
    for(const auto &i: sorted)
        out << '\"' << i.first << "\"," << i.second.first << ',' << i.second.second << '\n';

    out.close();  // done writing
}

bool LexiconBuilder::load_from_csv(const std::string& csv_path)
{
    std::ifstream file(csv_path);
    if (!file.is_open()) 
    {
        std::cerr<<"Error! cannot open file: "<<csv_path<<" for reading\n";
        return false; // file could not be opened
    }

    clear_lexicon(); // clear previous lexicon so no old data remains

    std::string line;
    bool first_line = true;
    uint32_t max_id = 0;  // used to update next_word_id correctly

    // read file line by line
    while (std::getline(file, line)) {
        if (first_line) { first_line = false; continue; } // skip header line
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string word, word_id_str, freq_str;

        // extract CSV columns
        if (!std::getline(ss, word, ',') ||
            !std::getline(ss, word_id_str, ',') ||
            !std::getline(ss, freq_str, ',')) 
        {
            continue; // skip malformed lines
        }

        uint32_t word_id = 0;
        uint32_t freq = 0;
        std::stringstream id_stream(word_id_str);
        std::stringstream freq_stream(freq_str);

        // parse numeric values
        if (!(id_stream >> word_id)) continue; // if parsing fails, skip line
        if (!(freq_stream >> freq)) continue;

        // store the parsed data
        lexicon_data[word] = {word_id, freq};

        // keep track of the highest id so new words get correct ids later
        if (word_id > max_id) max_id = word_id;
    }

    file.close();

    // after loading, set next id to max_id + 1 so new entries remain consistent
    next_word_id = max_id + 1;

    return true;
}

std::unordered_map<uint32_t,std::string> LexiconBuilder::build_reverse_lexicon()
{
    std::unordered_map<uint32_t,std::string> reverse_lexicon;
   
    for(auto& [word,details]:lexicon_data)
        reverse_lexicon[details.first]=word;

    return reverse_lexicon;
}

void LexiconBuilder::clear_lexicon()
{
    // simply resets the entire lexicon and id counter
    lexicon_data.clear();
    next_word_id = 0;
}
