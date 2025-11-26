#include "metadataparser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

MetadataParser::MetadataParser(const std::string& metadata_path) 
    : metadata_path(metadata_path) {}

bool MetadataParser::parse() {
    std::ifstream file(metadata_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open metadata.csv" << std::endl;
        return false;
    }
    
    std::string line;
    // Skip header line
    std::getline(file, line);
    
    int line_count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> fields = splitCSVLine(line);
        
        // CORD-19 metadata.csv has these columns (approximate):
        // cord_uid, sha, source_x, title, doi, pmcid, pubmed_id, license,
        // abstract, publish_time, authors, journal, url, ...
        
        if (fields.size() < 10) continue;
        
        Paper paper;
        paper.cord_uid = cleanField(fields[0]);
        paper.sha = cleanField(fields[1]);
        paper.source_x = cleanField(fields[2]);
        paper.title = cleanField(fields[3]);
        paper.doi = cleanField(fields[4]);
        paper.pmcid = cleanField(fields[5]);
        paper.pubmed_id = cleanField(fields[6]);
        paper.license = cleanField(fields[7]);
        paper.abstract_text = cleanField(fields[8]);
        paper.publish_time = cleanField(fields[9]);
        
        if (fields.size() > 10) paper.authors = cleanField(fields[10]);
        if (fields.size() > 11) paper.journal = cleanField(fields[11]);
        
        papers.push_back(paper);
        
        line_count++;
        if (line_count % 5000 == 0) {
            std::cout << "Parsed " << line_count << " papers..." << std::endl;
        }
    }
    
    file.close();
    std::cout << "Total papers parsed: " << papers.size() << std::endl;
    return true;
}

std::vector<std::string> MetadataParser::splitCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field); // Last field
    
    return fields;
}

std::string MetadataParser::cleanField(const std::string& field) {
    std::string cleaned = field;
    
    // Remove leading/trailing whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    // Remove quotes if present
    if (!cleaned.empty() && cleaned.front() == '"' && cleaned.back() == '"') {
        cleaned = cleaned.substr(1, cleaned.length() - 2);
    }
    
    return cleaned;
}