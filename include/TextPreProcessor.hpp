#pragma once
#include <string>
#include <vector>

class TextPreProcessor
{
    //class that will handle lower casing, punctuation removal and tokenization
    public:
    static std::vector<std::string> tokenize(const std:: string& text);

};