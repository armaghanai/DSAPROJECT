#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

class InvertedIndex
{
    private:
        // Original member
        std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>> inverted_index;
        
        // ===== NEW: BARREL SUPPORT =====
        
        // Barrel metadata structure
        struct BarrelMetadata {
            uint32_t barrel_id;
            uint32_t start_word_id;
            uint32_t end_word_id;
            std::string barrel_filename;
        };
        
        std::vector<BarrelMetadata> barrel_metadata;  // Stores info about all barrels
        std::string barrel_directory;                  // Directory where barrels are stored
        int currently_loaded_barrel;                   // Which barrel is currently in memory (-1 = none)
        
        // Helper: Find which barrel contains a word_id
        int find_barrel_index(uint32_t word_id) const;
        
        // Helper: Load a specific barrel by index
        bool load_barrel_by_index(int barrel_idx, 
                                 std::unordered_map<uint32_t, std::string>& reverse_lex);

    public:
        // ===== ORIGINAL METHODS (UNCHANGED) =====
        InvertedIndex();
        void add_document(uint32_t doc_id, 
            const std::vector<std::pair<uint32_t,uint32_t>>& terms);
        const std::vector<std::pair<uint32_t,uint32_t>>* get_terms(uint32_t word_id);
        void save_to_csv(const std::string& file_path, const std::unordered_map<uint32_t, std::string>& reverse_lex) const;
        void save_first_n_to_csv(const std::string& file_path,
            const std::unordered_map<uint32_t, std::string>& reverse_lex,size_t num)const;
        std::unordered_map<uint32_t,std::vector<std::pair<uint32_t,uint32_t>>> get_inverted_index()const;
        void save_to_binary(const std::string& file_path, const std::unordered_map<uint32_t, std::string>& reverse_lex) const;
        bool load_from_binary(const std::string& file_path, std::unordered_map<uint32_t, std::string>& reverse_lex);
        void clear();
        void print_statistics() const;
        
        // ===== NEW: BARREL METHODS =====
        
        // Create 4 barrels from current inverted_index
        // Splits words into 4 equal ranges by word_id
        bool create_barrels(const std::string& barrel_dir,
                           const std::unordered_map<uint32_t, std::string>& reverse_lex,
                           uint32_t num_barrels = 4);
        
        // Load barrel metadata (small file with ranges)
        // Call this once at startup instead of loading entire index
        bool load_barrel_metadata(const std::string& barrel_dir);
        
      
        bool load_barrel_for_word(uint32_t word_id,
                                 std::unordered_map<uint32_t, std::string>& reverse_lex);
        
        // Get currently loaded barrel info (-1 if none loaded)
        int get_loaded_barrel() const { return currently_loaded_barrel; }
        
        // Print barrel statistics
        void print_barrel_info() const;
};