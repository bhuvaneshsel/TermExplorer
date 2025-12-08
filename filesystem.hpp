#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H
#include "disk.hpp"
#include <cstdint>
#include <cstring>


enum class InodeType {
    UNUSED,
    FILE,
    DIRECTORY
};

struct Inode {
    InodeType type {InodeType::UNUSED};
    int index_block{};
    // int size{};
};

struct Superblock {
    int id{};          
    int total_blocks{};   
    int block_size{};     

    int inode_table_start{}; 
    int inode_table_blocks{};

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

    const Superblock& superblock() const { return m_superblock; };
    int max_inodes() const { return m_max_inodes; };
    const std::vector<Inode>& inode_table() const { return m_inode_table; };

private:
    Disk&       m_disk;
    Superblock  m_superblock{};
    std::vector<Inode> m_inode_table{};
    const int m_max_inodes{};
    bool initialize_superblock();
    bool initialize_inode_table();
    bool initialize_root_directory();
    bool read_superblock_from_disk();
    bool read_inode_table_from_disk();
};

#endif
