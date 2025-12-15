#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../include/WordEmbeddingsEngine.hpp"


namespace fs = std::filesystem;

 // Normalize vector to unit length (L2 norm)
void WordEmbeddingsEngine:: normalize_vector(std::vector<float>& vec) const {
        float norm = 0.0f;
        for (float val : vec) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm < 1e-10) return; // Avoid division by zero
        
        for (auto& val : vec) {
            val /= norm;
        }
    }



WordEmbeddingsEngine::WordEmbeddingsEngine() : embedding_dimension(0) {}

// Add this method to WordEmbeddingsEngine.hpp (in the public section)

// Save loaded embeddings to binary format
bool WordEmbeddingsEngine::save_to_binary(const std::string& output_file) const {
    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << output_file << " for writing" << std::endl;
        return false;
    }
    
    std::cout << "Saving embeddings to binary format: " << output_file << std::endl;
    
    // Write header: number of words and dimension
    uint32_t num_words = word_embeddings.size();
    uint32_t dim = embedding_dimension;
    
    out.write(reinterpret_cast<const char*>(&num_words), sizeof(num_words));
    out.write(reinterpret_cast<const char*>(&dim), sizeof(dim));
    
    std::cout << "Writing " << num_words << " embeddings (dimension: " << dim << ")..." << std::endl;
    
    int count = 0;
    for (const auto& [word, embedding] : word_embeddings) {
        // Write word length and word
        uint32_t word_len = word.length();
        out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));
        out.write(word.c_str(), word_len);
        
        // Write embedding vector
        out.write(reinterpret_cast<const char*>(embedding.data()), 
                 embedding.size() * sizeof(float));
        
        count++;
        if (count % 5000 == 0) {
            std::cout << "  Saved " << count << "/" << num_words << " embeddings..." << std::endl;
        }
    }
    
    out.close();
    
    
    uint64_t file_size_binary = fs::file_size(output_file);
    std::cout << "Saved to binary "<<std::endl;
    std::cout << "  File size: " << (file_size_binary / (1024.0 * 1024.0)) << " MB" << std::endl;
    std::cout << "  Words saved: " << num_words << std::endl;
    std::cout << "  Dimension: " << dim << std::endl;
    
    return true;
}
    
    // Load pre-trained embeddings (GloVe, Word2Vec, FastText format)
    // Format: word value1 value2 value3 ... valueN
bool WordEmbeddingsEngine::load_embeddings(const std::string& embedding_file) {
        std::ifstream file(embedding_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open embedding file " << embedding_file << std::endl;
            return false;
        }
        
        std::string line;
        int count = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            std::string word;
            
            if (!(iss >> word)) continue;
            
            std::vector<float> embedding;
            float value;
            
            while (iss >> value) {
                embedding.push_back(value);
            }
            
            if (embedding.empty()) continue;
            
            // Set dimension from first word
            if (embedding_dimension == 0) {
                embedding_dimension = embedding.size();
                std::cout << "Embedding dimension: " << embedding_dimension << std::endl;
            }
            
            // Normalize embedding to unit length
            normalize_vector(embedding);
            word_embeddings[word] = embedding;
            
            count++;
            if (count % 10000 == 0) {
                std::cout << "Loaded " << count << " embeddings..." << std::endl;
            }
        }
        
        file.close();
        std::cout << "✓ Loaded " << word_embeddings.size() << " word embeddings" << std::endl;
        return true;
    }
    
    // Get embedding for a single word
const std::vector<float>* WordEmbeddingsEngine:: get_word_embedding(const std::string& word) const 
    {
        auto it = word_embeddings.find(word);
        if (it != word_embeddings.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Compute average embedding for multiple words (document/query representation)
std::vector<float> WordEmbeddingsEngine:: get_average_embedding(const std::vector<std::string>& words) 
{

        std::vector<float> avg_embedding(embedding_dimension, 0.0f);
        int found_count = 0;
        
        for (const auto& word : words) {
            const auto* emb = get_word_embedding(word);
            if (emb) {
                for (int i = 0; i < embedding_dimension; i++) {
                    avg_embedding[i] += (*emb)[i];
                }
                found_count++;
            }
        }
        
        if (found_count > 0) {
            for (int i = 0; i < embedding_dimension; i++) {
                avg_embedding[i] /= found_count;
            }
            normalize_vector(avg_embedding);
        }
        
        return avg_embedding;
    }
    
    // Compute cosine similarity between two vectors
float WordEmbeddingsEngine :: cosine_similarity(const std::vector<float>& vec1, 
                           const std::vector<float>& vec2) const {
        if (vec1.size() != vec2.size() || vec1.empty()) {
            return 0.0f;
        }
        
        float dot_product = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < vec1.size(); i++) {
            dot_product += vec1[i] * vec2[i];
            norm1 += vec1[i] * vec1[i];
            norm2 += vec2[i] * vec2[i];
        }
        
        // Vectors should already be normalized, but be safe
        float denominator = std::sqrt(norm1) * std::sqrt(norm2);
        if (denominator < 1e-10) return 0.0f;
        
        return dot_product / denominator;
    }
    
int WordEmbeddingsEngine:: get_dimension() const {
        return embedding_dimension;
    }
    
size_t WordEmbeddingsEngine:: get_vocabulary_size() const {
        return word_embeddings.size();
}

bool WordEmbeddingsEngine::load_embeddings_binary (const std::string& binary_file) {
        
        std::ifstream file(binary_file, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open binary embedding file" << std::endl;
            return false;
        }
        
        // Read header
        uint32_t num_words, dim;
        file.read(reinterpret_cast<char*>(&num_words), sizeof(num_words));
        file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
        
        embedding_dimension = dim;
        std::cout << "Loading " << num_words << " embeddings (dim=" << dim << ")..." << std::endl;
        
        for (uint32_t i = 0; i < num_words; i++) {
            // Read word length and word
            uint32_t word_len;
            file.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));
            
            std::string word(word_len, '\0');
            file.read(&word[0], word_len);
            
            // Read embedding
            std::vector<float> embedding(dim);
            file.read(reinterpret_cast<char*>(embedding.data()), dim * sizeof(float));
            
            word_embeddings[word] = embedding;
            loaded_words.insert(word);
            
            if ((i + 1) % 5000 == 0) {
                std::cout << "  Loaded " << (i + 1) << "/" << num_words << std::endl;
            }
        }
        
        file.close();
        
        
        std::cout << "Loaded " << num_words << " embeddings in ";
        
        return true;
    }

bool WordEmbeddingsEngine::load_embeddings_for_lexicon(
        const std::string& embedding_file,
        const std::unordered_set<std::string>& lexicon_words) {
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::ifstream file(embedding_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open embedding file " << embedding_file << std::endl;
            return false;
        }
        
        std::string line;
        int total_lines = 0;
        int loaded = 0;
        int not_found = 0;
        
        std::cout << "\n=== Loading Embeddings for Lexicon ===" << std::endl;
        std::cout << "Target words: " << lexicon_words.size() << std::endl;
        std::cout << "Reading from: " << embedding_file << std::endl;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            std::string word;
            
            if (!(iss >> word)) continue;
            
            total_lines++;
            
            // OPTIMIZATION: Skip if word not in lexicon
            if (lexicon_words.find(word) == lexicon_words.end()) {
                not_found++;
                continue;
            }
            
            std::vector<float> embedding;
            float value;
            
            while (iss >> value) {
                embedding.push_back(value);
            }
            
            if (embedding.empty()) continue;
            
            // Set dimension from first word
            if (embedding_dimension == 0) {
                embedding_dimension = embedding.size();
                std::cout << "Embedding dimension detected: " << embedding_dimension << std::endl;
            }
            
            // Normalize embedding to unit length
            normalize_vector(embedding);
            word_embeddings[word] = embedding;
            loaded_words.insert(word);
            loaded++;
            
            // Progress output
            if (loaded % 2000 == 0) {
                float progress = (loaded * 100.0f) / lexicon_words.size();
                std::cout << "  Progress: " << loaded << "/" << lexicon_words.size() 
                          << " (" << std::fixed << std::setprecision(1) << progress << "%)" << std::endl;
            }
        }
        
        file.close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\n=== Loading Complete ===" << std::endl;
        std::cout << "✓ Loaded: " << loaded << " embeddings" << std::endl;
        std::cout << "✗ Not found: " << not_found << " words (not in GloVe)" << std::endl;
        std::cout << "  Scanned: " << total_lines << " lines" << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Coverage: " << std::fixed << std::setprecision(1) 
                  << (loaded * 100.0f / lexicon_words.size()) << "%" << std::endl;
        std::cout << "==============================\n" << std::endl;
        
        return loaded > 0;
    }

   