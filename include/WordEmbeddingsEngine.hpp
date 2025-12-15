#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

class WordEmbeddingsEngine {
private:
    // word -> embedding vector (e.g., 300-dim for GloVe)
    std::unordered_map<std::string, std::vector<float>> word_embeddings;
    int embedding_dimension;
     // For fast lookup without float vectors
    std::unordered_set<std::string> loaded_words;
    
public:
    WordEmbeddingsEngine();
    
    // Load pre-trained embeddings (GloVe, Word2Vec, FastText format)
    // Format: word value1 value2 value3 ... valueN
    bool load_embeddings(const std::string& embedding_file);
    // Get embedding for a single word
    const std::vector<float>* get_word_embedding(const std::string& word) const;
    
    // Compute average embedding for multiple words (document/query representation)
    std::vector<float> get_average_embedding(const std::vector<std::string>& words);
    
    // Compute cosine similarity between two vectors
    float cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const;
    
    int get_dimension() const;
    
    size_t get_vocabulary_size() const;
    bool save_to_binary(const std::string& output_file) const; 
    bool load_embeddings_for_lexicon(
        const std::string& embedding_file,
        const std::unordered_set<std::string>& lexicon_words);

      bool load_embeddings_binary(const std::string& binary_file);
    
private:
     // Normalize vector to unit length (L2 norm)
    void normalize_vector(std::vector<float>& vec) const;

};

