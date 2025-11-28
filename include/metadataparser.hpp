#pragma once
#include <string>
#include <vector>

struct Paper {
    std::string paper_id;      // SHA or PMC ID
    std::string title;
    std::string authors;
    std::string publish_date;
    std::string abstract_text;
    std::string body_text;     // Full body content
};

class MetadataParser 
{
public:
    const std::string data_path;
    std::vector<Paper> papers;
    
    // CSV parsing helpers
    static void parse_csv_line(const std::string& line, 
                               std::vector<std::string>& parsed_line);
    static std::string clean_field(const std::string& field);
    
    // Full-text extraction from JSON files
    std::string find_fulltext_pdf(const std::string& sha);
    std::string find_fulltext_xml(const std::string& pmcid);
    
    // Extract body text from JSON
    static std::string extract_body_from_json(const std::string& json_path);
    
    // Extract and save body text to file (optional)
    static void extract_body_text_tofile(const std::string& file_path,
                                        std::ofstream& output_file); 

public:
    explicit MetadataParser(const std::string& data_path) 
        : data_path(data_path) {}
    
    // Get paper count from metadata.csv
    int metadata_stats();
    
    // Main parsing function - extracts metadata + full body text
    int metadata_parse();
    
    // Get parsed papers
    const std::vector<Paper>& getPapers() const { return papers; }
    size_t getCount() const { return papers.size(); }
};