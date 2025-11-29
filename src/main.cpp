#include "../include/metadataparser.hpp"
#include "../include/TextPreProcessor.hpp"
#include "../include/LexiconBuilder.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main() {
    // Step 1: Set dataset path
    // absolute path of the dataset folder containing metadata + JSON files
    std::string dataset_path = "D:/3RD SEMESTER/Data Structures and Algorithms/dsa search engine/DSA-Searh-Engine/data/2020-04-10" ;
    MetadataParser parser(dataset_path);

    // Step 2: Parse metadata
    // get how many entries exist in metadata.csv (basic statistics)
    std::cout << "Counting metadata entries..." << std::endl;
    int total_entries = parser.metadata_stats();
    std::cout << "Total metadata entries: " << total_entries << std::endl;

    // actually parse metadata and extract titles, body text, file paths etc.
    std::cout << "Parsing metadata and extracting papers..." << std::endl;
    parser.metadata_parse();
    std::cout << "Total papers parsed: " << parser.papers.size() << std::endl;

    // Step 3: Text preprocessing setup
    // object used for tokenizing, cleaning, lowercasing etc.
    TextPreprocessor preprocessor;

    // Step 4: Build lexicon
    // lexicon maps each unique token -> (word_id, frequency)
    LexiconBuilder lexicon;

    for (const auto& paper : parser.papers) {
        // preprocess body text to convert it into tokens
        std::vector<std::string> tokens = preprocessor.preprocess(paper.body_text);

        // add each token to the lexicon (each counts as 1 occurrence)
        for (const auto& token : tokens) {
            lexicon.add_word(token, 1);
        }
    }

    // display how many unique words were processed
    std::cout << "Lexicon size: " << lexicon.get_size() << " unique words" << std::endl;

    // Step 5: Save lexicon to CSV
    // directory where lexicon.csv will be written
    std::string filepath = "D:/3RD SEMESTER/Data Structures and Algorithms/dsa search engine/DSA-Searh-Engine/indices/";
    std::string filename = "lexicon.csv";

    // write the sorted lexicon to a CSV file
    lexicon.save_to_csv(filepath + filename);
    std::cout << "Lexicon saved to lexicon.csv\n";

    // Step 6: Test loading lexicon from CSV
    // load again to verify reading logic works correctly
    std::cout << "Loading lexicon from file\n";

    if (lexicon.load_from_csv(filepath + filename))
        std::cout << "Data loaded successfully:\n";
    else
        std::cout << "Error Occured in loading\n";

    return 0;
}
