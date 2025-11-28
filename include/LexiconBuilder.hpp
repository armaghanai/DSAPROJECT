#pragma once
#include <vector>
#include <string>
#include <map>

class LexiconBuilder
{
    private:
    //the words and list of doc_ids they belong to
    // are stored as key-value pairs in lexicon
    std::map<std::string,std::vector<std::string>> lexicon;
    //pair stores the doc id and the text
    std::vector<std::pair<std::string,std::string>> documents;

    public:
    //parses all docs and adds them to lexicon
    void addDocument(const std::string& doc_id,const std::string& text);
    //processing using tokenization
    void build();
    //printing the lexicon
    void displayLexicon()const;
};