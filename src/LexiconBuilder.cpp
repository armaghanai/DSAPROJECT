#include "../include/LexiconBuilder.hpp"
#include "../include/TextPreProcessor.hpp"
#include <iostream>
#include <algorithm>

void LexiconBuilder::addDocument(const std::string& doc_id,const std::string& text)
{
    //stores the details of doc as a vector
    documents.push_back({doc_id,text});
}

void LexiconBuilder::build()
{
    lexicon.clear();
    for(auto&[doc_id,text]:documents)
    {
        auto tokens = TextPreProcessor::tokenize(text);
        for(auto& token:tokens)
        {
            lexicon[token].push_back(doc_id);
        }
    }
}

void LexiconBuilder::displayLexicon() const
{
    // unordered_map has no order, sort keys alphabetically for display
    std::vector<std::string> words;
    for (auto& [word, _] : lexicon)
        words.push_back(word);

    std::sort(words.begin(), words.end());

    for (auto& word : words)
    {
        std::cout << word << ": ";
        for (auto& id : lexicon.at(word))
            std::cout << id << ' ';
        std::cout << '\n';
    }
}