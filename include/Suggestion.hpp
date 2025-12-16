#include <string>
#include <cstdint>

struct Suggestion {
    std::string word;
    uint32_t frequency;
    double relevance_score;
    
    Suggestion(const std::string& w, uint32_t freq, double score = 1.0)
        : word(w), frequency(freq), relevance_score(score) {}
};