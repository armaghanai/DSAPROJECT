#include "metadataparser.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main() {
    // ======== Step 1: Set dataset path ========
    //absolute path of device
    std::string dataset_path = "D:/3RD SEMESTER/Data Structures and Algorithms/dsa search engine/DSA-Searh-Engine/data/2020-04-10" ;// Change this to your CORD-19 dataset folder
    MetadataParser parser(dataset_path);

    // ======== Step 2: Parse metadata ========
    std::cout << "Counting metadata entries..." << std::endl;
    int total_entries = parser.metadata_stats();
    std::cout << "Total metadata entries: " << total_entries << std::endl;

    std::cout << "Parsing metadata and extracting papers..." << std::endl;
    parser.metadata_parse();
    std::cout << "Total papers parsed: " << parser.papers.size() << std::endl;

    // ======== Step 3: Print full body text of first few papers ========
    int num_to_show = 8000; // change how many papers you want to see
    std::cout << "\n=== Showing full text of first " << num_to_show << " papers ===\n" << std::endl;

    for (int i = 0; i < num_to_show; i+=800) {
        const Paper& paper = parser.papers[i];
        std::cout << "Paper ID: " << paper.paper_id << std::endl;
        std::cout << "Title: " << paper.title << std::endl;
        std::cout << "Publish Date: " << paper.publish_date << std::endl;
        std::cout << "Authors: " << paper.authors << std::endl;
        std::cout << (paper.body_text.empty() ? "[No Text]" : paper.body_text.substr(0, 500)) << "\n...";

    }

    return 0;
}

