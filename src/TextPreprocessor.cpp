#include "../include/TextPreProcessor.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

vector<string>TextPreProcessor::tokenize(const string& text)
{
    string clean_text;
    clean_text.reserve(text.size());
    for(unsigned char c: text)
    {
        //adds the alphanumeric characters and spaces to 
        //clean text in their lower case form
        if(isalnum(c)||isspace(c))
            clean_text += tolower(c);
        //else
            //clean_text += ' '; //replace punctuation with space
    }
    vector<string> tokens;
    //converts the clean text into an input stream
    istringstream iss(clean_text);
    string token;
    //converts the the stream into clean tokenized  vector of strings
    while(iss >> token)
        tokens.push_back(token);

    return tokens;
}