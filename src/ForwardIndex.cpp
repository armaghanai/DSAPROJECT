#include "../include/ForwardIndex.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>

ForwardIndex::ForwardIndex() 
    : next_doc_id(0), total_documents(0), total_terms(0) {}

void ForwardIndex::add_document(const std::string& doc_id,
                                const std::string& title,
                                const std::string& abstract_text,
                                const std::vector<uint32_t>& word_ids) {
    
    // Check if document already exists
    if (forward_index.find(doc_id) != forward_index.end()) {
        std::cerr << "Warning: Document " << doc_id << " already exists. Skipping." << std::endl;
        return;
    }
    
    DocumentIndex doc_index;
    doc_index.doc_id = doc_id;
    doc_index.title = title;
    doc_index.abstract_text = abstract_text;
    doc_index.doc_length = word_ids.size();
    
    // Create term frequency map: word_id -> frequency
    std::unordered_map<uint32_t, uint32_t> term_map;
    
    for (uint32_t word_id : word_ids) {
        term_map[word_id]++;
    }
    
    // Convert term_map to vector of TermPosting
    doc_index.terms.reserve(term_map.size());
    for (const auto& entry : term_map) {
        TermPosting posting;
        posting.word_id = entry.first;
        posting.frequency = entry.second;
        doc_index.terms.push_back(posting);
    }
    
    // Sort terms by word_id for efficient lookup
    std::sort(doc_index.terms.begin(), doc_index.terms.end(),
              [](const TermPosting& a, const TermPosting& b) {
                  return a.word_id < b.word_id;
              });
    
    // Add to forward index
    forward_index[doc_id] = doc_index;
    doc_id_map[doc_id] = next_doc_id++;
    
    // Update statistics
    total_documents++;
    total_terms += word_ids.size();
}

const DocumentIndex* ForwardIndex::get_document(const std::string& doc_id) const {
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return &(it->second);
    }
    return nullptr;
}

const std::vector<TermPosting>* ForwardIndex::get_document_terms(const std::string& doc_id) const {
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return &(it->second.terms);
    }
    return nullptr;
}

uint32_t ForwardIndex::get_document_length(const std::string& doc_id) const {
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return it->second.doc_length;
    }
    return 0;
}

uint32_t ForwardIndex::get_term_frequency(const std::string& doc_id, uint32_t word_id) const {
    auto it = forward_index.find(doc_id);
    if (it == forward_index.end()) {
        return 0;
    }
    
    // Binary search since terms are sorted by word_id
    const auto& terms = it->second.terms;
    auto term_it = std::lower_bound(terms.begin(), terms.end(), word_id,
                                     [](const TermPosting& posting, uint32_t id) {
                                         return posting.word_id < id;
                                     });
    
    if (term_it != terms.end() && term_it->word_id == word_id) {
        return term_it->frequency;
    }
    
    return 0;
}

bool ForwardIndex::save_to_binary(const std::string& file_path) {
    std::ofstream out(file_path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open file " << file_path << " for writing" << std::endl;
        return false;
    }
    
    // Write header: total_documents, total_terms, next_doc_id
    out.write(reinterpret_cast<const char*>(&total_documents), sizeof(total_documents));
    out.write(reinterpret_cast<const char*>(&total_terms), sizeof(total_terms));
    out.write(reinterpret_cast<const char*>(&next_doc_id), sizeof(next_doc_id));
    
    // Write number of documents
    uint32_t num_docs = forward_index.size();
    out.write(reinterpret_cast<const char*>(&num_docs), sizeof(num_docs));
    
    // Write each document
    for (const auto& entry : forward_index) {
        const DocumentIndex& doc = entry.second;
        
        // Write doc_id (string)
        uint32_t doc_id_len = doc.doc_id.length();
        out.write(reinterpret_cast<const char*>(&doc_id_len), sizeof(doc_id_len));
        out.write(doc.doc_id.c_str(), doc_id_len);
        
        // Write title (string)
        uint32_t title_len = doc.title.length();
        out.write(reinterpret_cast<const char*>(&title_len), sizeof(title_len));
        out.write(doc.title.c_str(), title_len);
        
        // Write abstract_text (string)
        uint32_t abstract_len = doc.abstract_text.length();
        out.write(reinterpret_cast<const char*>(&abstract_len), sizeof(abstract_len));
        out.write(doc.abstract_text.c_str(), abstract_len);
        
        // Write doc_length
        out.write(reinterpret_cast<const char*>(&doc.doc_length), sizeof(doc.doc_length));
        
        // Write number of unique terms
        uint32_t num_terms = doc.terms.size();
        out.write(reinterpret_cast<const char*>(&num_terms), sizeof(num_terms));
        
        // Write each term posting (word_id and frequency only)
        for (const auto& term : doc.terms) {
            out.write(reinterpret_cast<const char*>(&term.word_id), sizeof(term.word_id));
            out.write(reinterpret_cast<const char*>(&term.frequency), sizeof(term.frequency));
        }
    }
    
    out.close();
    std::cout << "Forward index saved to " << file_path << std::endl;
    return true;
}

bool ForwardIndex::load_from_binary(const std::string& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open file " << file_path << " for reading" << std::endl;
        return false;
    }
    
    clear();
    
    // Read header
    in.read(reinterpret_cast<char*>(&total_documents), sizeof(total_documents));
    in.read(reinterpret_cast<char*>(&total_terms), sizeof(total_terms));
    in.read(reinterpret_cast<char*>(&next_doc_id), sizeof(next_doc_id));
    
    // Read number of documents
    uint32_t num_docs;
    in.read(reinterpret_cast<char*>(&num_docs), sizeof(num_docs));
    
    // Read each document
    for (uint32_t i = 0; i < num_docs; ++i) {
        DocumentIndex doc;
        
        // Read doc_id
        uint32_t doc_id_len;
        in.read(reinterpret_cast<char*>(&doc_id_len), sizeof(doc_id_len));
        doc.doc_id.resize(doc_id_len);
        in.read(&doc.doc_id[0], doc_id_len);
        
        // Read title
        uint32_t title_len;
        in.read(reinterpret_cast<char*>(&title_len), sizeof(title_len));
        doc.title.resize(title_len);
        in.read(&doc.title[0], title_len);
        
        // Read abstract_text
        uint32_t abstract_len;
        in.read(reinterpret_cast<char*>(&abstract_len), sizeof(abstract_len));
        doc.abstract_text.resize(abstract_len);
        in.read(&doc.abstract_text[0], abstract_len);
        
        // Read doc_length
        in.read(reinterpret_cast<char*>(&doc.doc_length), sizeof(doc.doc_length));
        
        // Read number of unique terms
        uint32_t num_terms;
        in.read(reinterpret_cast<char*>(&num_terms), sizeof(num_terms));
        doc.terms.resize(num_terms);
        
        // Read each term posting
        for (uint32_t j = 0; j < num_terms; ++j) {
            TermPosting& term = doc.terms[j];
            in.read(reinterpret_cast<char*>(&term.word_id), sizeof(term.word_id));
            in.read(reinterpret_cast<char*>(&term.frequency), sizeof(term.frequency));
        }
        
        forward_index[doc.doc_id] = doc;
        doc_id_map[doc.doc_id] = i;
    }
    
    in.close();
    std::cout << "Forward index loaded from " << file_path << std::endl;
    return true;
}

void ForwardIndex::save_to_csv(const std::string& file_path)
{
    std::ofstream out(file_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open file " << file_path << " for writing\n";
        return;
    }

    // CSV header
    out << "doc_id,word_id,frequency\n";

    for (const auto& [doc_id, doc] : forward_index) {

        for (const auto& term : doc.terms) {
            out << doc.doc_id << "," 
                << term.word_id << ","
                << term.frequency << "\n";
        }
    }

    out.close();
    std::cout << "CSV for forward index saved to " << file_path << "\n";
}
std::unordered_map<std::string, uint32_t> ForwardIndex::get_doc_id_map()
{
    return doc_id_map;
}
void ForwardIndex::save_first_n_to_csv(const std::string& file_path, size_t number_of_docs)
{
    std::ofstream out(file_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open file " << file_path << " for writing\n";
        return;
    }

    // CSV header
    out << "doc_id,word_id,frequency\n";

    size_t count = 0;
    for (const auto& [doc_id, doc] : forward_index) {
        if (count >= number_of_docs)
            break;

        for (const auto& term : doc.terms) {
            out << doc.doc_id << "," 
                << term.word_id << ","
                << term.frequency << "\n";
        }

        count++;
    }

    out.close();
    std::cout << "CSV for first " << number_of_docs << " docs saved to " << file_path << "\n";
}

void ForwardIndex::clear() {
    forward_index.clear();
    doc_id_map.clear();
    next_doc_id = 0;
    total_documents = 0;
    total_terms = 0;
}

void ForwardIndex::print_statistics() const 
{
    std::cout << "\n=== Forward Index Statistics ===" << std::endl;
    std::cout << "Total documents: " << total_documents << std::endl;
    std::cout << "Total terms (with repetition): " << total_terms << std::endl;
    std::cout << "Average document length: " << std::fixed << std::setprecision(2) 
              << get_average_doc_length() << " terms" << std::endl;
    
    // Calculate average unique terms per document
    uint64_t total_unique_terms = 0;
    for (const auto& entry : forward_index) {
        total_unique_terms += entry.second.terms.size();
    }
    
    if (total_documents > 0) {
        std::cout << "Average unique terms per document: " << std::fixed << std::setprecision(2)
                  << static_cast<double>(total_unique_terms) / total_documents << std::endl;
    }
    std::cout << "================================\n" << std::endl;
}

double ForwardIndex::get_average_doc_length() const {
    if (total_documents == 0) return 0.0;
    return static_cast<double>(total_terms) / total_documents;
}
