#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H
#include "disk.hpp"
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>


enum class InodeType {
    UNUSED,
    FILE,
    DIRECTORY
};

struct Inode {
    InodeType type {InodeType::UNUSED};
    int index_block{};
    int size{};
};

struct DirectoryEntry {
    int inode_index{};
    char name[56];
};

struct Superblock {
    int id{};          
    int total_blocks{};   
    int block_size{};     

    int inode_table_start{}; 
    int inode_table_blocks{}; //TODO: use?

    int free_bitmap_start{};
    int free_bitmap_blocks{};

    int data_region_start{};
    int root_inode_index{};
};

class FileSystem {
public:
    FileSystem(Disk& disk, int max_inodes) : m_disk(disk), m_max_inodes{max_inodes} {};


    bool initialize();
    bool mount();

    bool create_file(const std::string& name);
    bool write_file(const std::string& path, const std::string& data);
    bool read_file(const std::string& path, std::string& out);

    const Superblock& superblock() const { return m_superblock; };
    int max_inodes() const { return m_max_inodes; };
    const std::vector<Inode>& inode_table() const { return m_inode_table; };

    bool create_directory(const std::string& path);
private:
    Disk& m_disk;
    Superblock m_superblock{};
    std::vector<Inode> m_inode_table{};
    std::vector<uint8_t> m_free_bitmap{};

    const int m_max_inodes{};
    bool initialize_superblock();
    bool initialize_inode_table();
    bool initialize_root_directory();
    bool initialize_free_bitmap();

    bool mark_block_used(int block_number);

    bool read_superblock_from_disk();
    bool read_inode_table_from_disk();
    bool read_free_bitmap_from_disk();

    int allocate_block();               
    int allocate_inode();               
    bool write_inode_table_to_disk();    
    bool write_free_bitmap_to_disk();    
    
    bool add_directory_entry(int directory_inode_index, int inode_index, const std::string& name);
    int find_directory_entry(int directory_inode_index, const std::string& name);

    int resolve_path(const std::string& path);
    std::vector<std::string> split_path(const std::string& path);
    int resolve_parent_directory(const std::string& path, std::string& leaf);
};

#endif
