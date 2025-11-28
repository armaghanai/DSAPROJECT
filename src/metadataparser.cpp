#include "../include/metadataparser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

// For JSON parsing - you'll need nlohmann/json library
#include <nlohmann_json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

int MetadataParser::metadata_stats() {
    std::string metadata_path = data_path + "/metadata.csv";
    
    std::ifstream file(metadata_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << metadata_path << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    std::getline(file, line); // Skip header
    
    while (std::getline(file, line)) {
        if (!line.empty()) count++;
    }
    
    file.close();
    return count;
}

int MetadataParser::metadata_parse() {
    std::string metadata_path = data_path + "/metadata.csv";
    
    std::ifstream file(metadata_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << metadata_path << std::endl;
        return 0;
    }
    
    std::string line;
    std::getline(file, line); // Skip header
    
    int parsed_count = 0;
    int full_text_count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        parse_csv_line(line, fields);
        
        if (fields.size() < 10) continue;
        
        Paper paper;
        
        // Extract basic metadata
        // Assuming CSV format: cord_uid, sha, source_x, title, doi, pmcid, pubmed_id, license, abstract, publish_time, authors, journal...
        paper.paper_id = clean_field(fields[0]);     // cord_uid
        std::string sha = clean_field(fields[1]);     // sha
        paper.title = clean_field(fields[3]);         // title
        std::string pmcid = clean_field(fields[5]);   // pmcid
        paper.abstract_text = clean_field(fields[8]); // abstract
        paper.publish_date = clean_field(fields[9]);  // publish_time
        
        if (fields.size() > 10) {
            paper.authors = clean_field(fields[10]);  // authors
        }
        
        // Try to extract full body text from JSON files
        std::string full_text;
        
        // Try PDF JSON first (using SHA)
        if (!sha.empty()) {
            full_text = find_fulltext_pdf(sha);
        }
        
        // If not found, try PMC JSON
        if (full_text.empty() && !pmcid.empty()) {
            full_text = find_fulltext_xml(pmcid);
        }
        
        if (!full_text.empty()) {
            paper.body_text = full_text;
            full_text_count++;
        }

        /*std::cout << "Parsed paper: " << paper.paper_id
          << " | Title: " << paper.title << "\n";*/

        
        papers.push_back(paper);
        parsed_count++;
        //std::cout << paper.body_text << std::endl;

        
        if (parsed_count % 1000 == 0) {
            std::cout << "Parsed " << parsed_count << " papers (with full text: " 
                      << full_text_count << ")..." << std::endl;
        }
    }
    
    file.close();
    
    std::cout << "\nParsing complete!" << std::endl;
    std::cout << "Total papers: " << parsed_count << std::endl;
    std::cout << "Papers with full text: " << full_text_count << std::endl;
    
    return parsed_count;
}

void MetadataParser::parse_csv_line(const std::string& line, 
                                    std::vector<std::string>& parsed_line) {
    parsed_line.clear();
    std::string field;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            parsed_line.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    parsed_line.push_back(field);
}

std::string MetadataParser::clean_field(const std::string& field) {
    std::string cleaned = field;
    
    // Remove leading/trailing whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    // Remove surrounding quotes
    if (!cleaned.empty() && cleaned.front() == '"' && cleaned.back() == '"') {
        cleaned = cleaned.substr(1, cleaned.length() - 2);
    }
    
    return cleaned;
}

std::string MetadataParser::find_fulltext_pdf(const std::string& sha) {
    if (sha.empty()) return "";
    
    // CORD-19 structure: document_parses/pdf_json/{sha}.json
    std::string json_path = data_path + "/comm_use_subset/pdf_json/" + sha + ".json";
    
    if (fs::exists(json_path)) {
        return extract_body_from_json(json_path);
    }
    
    return "";
}

std::string MetadataParser::find_fulltext_xml(const std::string& pmcid) {
    if (pmcid.empty()) return "";
    
    // CORD-19 structure: document_parses/pmc_json/{pmcid}.xml.json
    std::string json_path = data_path + "/comm_use_subset/pmc_json/" + pmcid + ".xml.json";
    
    if (fs::exists(json_path)) {
        return extract_body_from_json(json_path);
    }
    
    return "";
}

std::string MetadataParser::extract_body_from_json(const std::string& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        return "";
    }
    
    try {
        json j;
        file >> j;
        
        std::string body_text;
        
        // Extract abstract sections
        if (j.contains("abstract")) {
            for (const auto& section : j["abstract"]) {
                if (section.contains("text")) {
                    body_text += section["text"].get<std::string>() + "\n\n";
                }
            }
        }
        
        // Extract body text sections
        if (j.contains("body_text")) {
            for (const auto& section : j["body_text"]) {
                if (section.contains("text")) {
                    body_text += section["text"].get<std::string>() + "\n\n";
                }
            }
        }
        
        return body_text;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON " << json_path << ": " << e.what() << std::endl;
        return "";
    }
}

void MetadataParser::extract_body_text_tofile(const std::string& file_path,
                                             std::ofstream& output_file) {
    std::string body_text = extract_body_from_json(file_path);
    
    if (!body_text.empty()) {
        output_file << body_text << "\n\n---END OF DOCUMENT---\n\n";
    }
}