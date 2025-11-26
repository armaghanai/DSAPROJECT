#ifndef METADATA_PARSER_HPP
#define METADATA_PARSER_HPP

#include <string>
#include <vector>
#include <map>

struct Paper {
    std::string cord_uid;
    std::string sha;
    std::string source_x;
    std::string title;
    std::string doi;
    std::string pmcid;
    std::string pubmed_id;
    std::string license;
    std::string abstract_text;
    std::string publish_time;
    std::string authors;
    std::string journal;
    std::string url;
};

class MetadataParser {
public:
    MetadataParser(const std::string& metadata_path);
    bool parse();
    const std::vector<Paper>& getPapers() const { return papers; }
    size_t getCount() const { return papers.size(); }
    
private:
    std::string metadata_path;
    std::vector<Paper> papers;
    
    std::vector<std::string> splitCSVLine(const std::string& line);
    std::string cleanField(const std::string& field);
};

#endif