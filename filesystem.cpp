#include "filesystem.hpp"
#include <iostream>

bool FileSystem::initialize() {
    if (!m_disk.is_open()) {
        std::cerr <<"Cannot format: disk is not open\n";
        return false;
    }

    if (!FileSystem::initialize_superblock()) {
        std::cerr << "Failed to intialize superblock\n";
        return false;
    }

    if (!FileSystem::initialize_inode_table()) {
        std::cerr << "Failed to initialize inode table\n";
        return false;
    }
    return true;
}

bool FileSystem::initialize_superblock() {

    m_superblock.id = 0;
    m_superblock.total_blocks = m_disk.number_of_blocks();
    m_superblock.block_size = m_disk.block_size();
    m_superblock.inode_table_start = 1;

    std::vector<char> buffer(m_disk.block_size(), 0);

    std::memcpy(buffer.data(), &m_superblock, sizeof(Superblock) < buffer.size() ? sizeof(Superblock) : buffer.size());

    if (!m_disk.write_block(0, buffer.data())) {
        std::cerr << "Failed to write superblock to disk\n";
        return false;
    } 

    return true;
}

bool FileSystem::initialize_inode_table() {
    const int block_size = {m_disk.block_size()};
    const int total_blocks {m_disk.number_of_blocks()};

    const int inode_table_bytes {m_max_inodes * static_cast<int>(sizeof(Inode))};
    const int inode_table_blocks {(inode_table_bytes + block_size - 1) / block_size};

    const int root_index_block_start {m_superblock.inode_table_start + inode_table_blocks};

    m_inode_table.assign(m_max_inodes, Inode{}); 

    Inode& root = m_inode_table[0];
    root.type = InodeType::DIRECTORY;
    root.index_block = root_index_block_start; 

    std::vector<char> buffer(inode_table_blocks * block_size, 0);
    std::memcpy(buffer.data(), m_inode_table.data(), inode_table_bytes);

    for (int i = 0; i < inode_table_blocks; ++i) {
        int block_number = m_superblock.inode_table_start + i;
        if (!m_disk.write_block(block_number, buffer.data() + i * block_size)) {
            std::cerr << "Failed to write inode table block " << block_number << "\n";
            return false;
        }
    }
    return true;
}


bool FileSystem::read_superblock_from_disk() {
    if (!m_disk.is_open()) {
        std::cerr << "Cannot read superblock: disk not open\n";
        return false;
    }

    std::vector<char> buffer(m_disk.block_size(), 0);

    if (!m_disk.read_block(0, buffer.data())) {
        std::cerr << "Failed to read block 0\n";
        return false;
    }

    std::memcpy(&m_superblock, buffer.data(), sizeof(Superblock));
    return true;
}

bool FileSystem::read_inode_table_from_disk() {
    const int block_size = {m_disk.block_size()};

    const int inode_table_bytes {m_max_inodes * static_cast<int>(sizeof(Inode))};
    const int inode_table_blocks {(inode_table_bytes + block_size - 1) / block_size};

    const int inode_table_start  = m_superblock.inode_table_start;
    const int inode_table_end    = inode_table_start + inode_table_blocks;

    std::vector<char> buffer(inode_table_blocks * block_size, 0);

    for (int i = 0; i < inode_table_blocks; ++i) {
        int block_number = inode_table_start + i;
        if (!m_disk.read_block(block_number, buffer.data() + i * block_size)) {
            std::cerr << "mount: failed to read inode table block " << block_number << "\n";
            return false;
        }
    }

    m_inode_table.assign(m_max_inodes, Inode{});
    std::memcpy(m_inode_table.data(), buffer.data(), inode_table_bytes);
    return true;
}


bool FileSystem::mount() {
    if (!m_disk.is_open()) {
        std::cerr << "Cannot mount: disk is not open\n";
        return false;
    }
    
    if (!FileSystem::read_superblock_from_disk()) {
        std::cerr << "Reading superblock from disk failed\n";
        return false;
    }

    if (!FileSystem::read_inode_table_from_disk()) {
        std::cerr <<"Reading inode table from disk failed\n";
        return false;
    }
    return true;
}


