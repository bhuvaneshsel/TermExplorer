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

    if (!FileSystem::initialize_free_bitmap()) {
        std::cerr << "Failed to initialize free bitmap\n";
        return false;
    }
    return true;
}

bool FileSystem::initialize_superblock() {
    
    const int block_size = m_disk.block_size();
    const int total_blocks = m_disk.number_of_blocks();

    const int inode_table_bytes  = m_max_inodes * static_cast<int>(sizeof(Inode));
    const int inode_table_blocks = (inode_table_bytes + block_size - 1) / block_size;

    const int bits_per_block = block_size * 8;
    const int bitmap_blocks  = (total_blocks + bits_per_block - 1) / bits_per_block;

    m_superblock.id = SUPERBLOCK_MAGIC;
    m_superblock.total_blocks = total_blocks;
    m_superblock.block_size = block_size;

    m_superblock.inode_table_start = 1;
    m_superblock.inode_table_blocks = inode_table_blocks;

    m_superblock.free_bitmap_start = m_superblock.inode_table_start + inode_table_blocks;
    m_superblock.free_bitmap_blocks = bitmap_blocks;

    m_superblock.data_region_start = m_superblock.free_bitmap_start + bitmap_blocks;

    m_superblock.root_inode_index  = 0;

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
    const int inode_table_blocks {m_superblock.inode_table_blocks};

    const int root_index_block_start {m_superblock.data_region_start};

    m_inode_table.assign(m_max_inodes, Inode{}); 
    FileSystem::initialize_root_directory();

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

bool FileSystem::initialize_root_directory() {
    Inode& root = m_inode_table[0];
    root.type = InodeType::DIRECTORY;
    root.index_block = m_superblock.data_region_start;
    root.size = 0;

    const int block_size = m_disk.block_size();

    std::vector<char> buffer(block_size, 0);
    DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(DirectoryEntry));

    for (int i = 0; i < max_entries; ++i) {
        entries[i].inode_index = -1;
        std::memset(entries[i].name, 0, sizeof(entries[i].name));
    }

    if (!m_disk.write_block(root.index_block, buffer.data())) {
        std::cerr << "Failed to write root directory block\n";
        return false;
    }
    return true;
}

bool FileSystem::initialize_free_bitmap() {
    const int block_size {m_disk.block_size()};
    const int total_blocks {m_disk.number_of_blocks()};

    const int bitmap_blocks {m_superblock.free_bitmap_blocks};
    m_free_bitmap.assign(bitmap_blocks * block_size, 0xFF);

    // Mark Superblock as used
    FileSystem::mark_block_used(0);

    // Make Inode Table Blocks as used
    for (int b = 0; b < m_superblock.inode_table_blocks; ++b) {
        FileSystem::mark_block_used(m_superblock.inode_table_start + b);
    }

    // Mark Bitmap Blocks as used
    for (int b = 0; b < bitmap_blocks; ++b)
        FileSystem::mark_block_used(m_superblock.free_bitmap_start + b);

    // Mark Root Directory Index Block as used
    FileSystem::mark_block_used(m_inode_table[0].index_block);
    
    //Write Free Bitmap to disk
    for (int i = 0; i < bitmap_blocks; ++i) {
        int block_number = m_superblock.free_bitmap_start + i;
        if (!m_disk.write_block(block_number, reinterpret_cast<const char*>(m_free_bitmap.data()) + i * block_size)) {
            std::cerr << "Failed to write free-space bitmap block " << block_number << "\n";
            return false;
        }
    }

    return true;
}

bool FileSystem::mark_block_used(int block_number) {
    const int total_blocks = m_disk.number_of_blocks();

    if (block_number < 0 || block_number >= total_blocks) {
        return false;
    }

    int byte_index = block_number / 8;
    int bit_index  = block_number % 8;

    m_free_bitmap[byte_index] &= ~(1u << bit_index);
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

bool FileSystem::read_free_bitmap_from_disk() {
    const int block_size   = m_disk.block_size();
    const int bitmap_blocks= m_superblock.free_bitmap_blocks;

    m_free_bitmap.assign(bitmap_blocks * block_size, 0);

    for (int i = 0; i < bitmap_blocks; ++i) {
        int block_number = m_superblock.free_bitmap_start + i;
        if (!m_disk.read_block(block_number, reinterpret_cast<char*>(m_free_bitmap.data()) + i * block_size)) {
            std::cerr << "Failed to read free-space bitmap block " << block_number << "\n";
            return false;
        }
    }
    return true;
}

bool FileSystem::write_inode_table_to_disk() {
    const int block_size = m_disk.block_size();
    const int inode_table_bytes = m_max_inodes * static_cast<int>(sizeof(Inode));
    const int inode_table_blocks = m_superblock.inode_table_blocks;

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


int FileSystem::allocate_block() {
    const int total_blocks = m_disk.number_of_blocks();

    for (int block = 0; block < total_blocks; ++block) {
        int byte_index = block / 8;
        int bit_index  = block % 8;

        if (byte_index >= static_cast<int>(m_free_bitmap.size()))
            break;

        uint8_t mask = static_cast<uint8_t>(1u << bit_index);
        if (m_free_bitmap[byte_index] & mask) {
            m_free_bitmap[byte_index] &= ~mask;

            if (!write_free_bitmap_to_disk()) {
                std::cerr << "Failed to persist free bitmap after allocating block\n";
                return -1;
            }
            return block;
        }
    }
    std::cerr << "No free blocks available\n";
    return -1;
}

int FileSystem::allocate_inode() {
    for (int i = 0; i < m_max_inodes; ++i) {
        if (m_inode_table[i].type == InodeType::UNUSED) {
            return i;
        }
    }
    std::cerr << "No free inodes available\n";
    return -1;
}

bool FileSystem::write_free_bitmap_to_disk() {
    const int block_size = m_disk.block_size();
    const int bitmap_blocks = m_superblock.free_bitmap_blocks;

    for (int i = 0; i < bitmap_blocks; ++i) {
        int block_number = m_superblock.free_bitmap_start + i;
        if (!m_disk.write_block(block_number, reinterpret_cast<const char*>(m_free_bitmap.data()) + i * block_size)) {
            std::cerr << "Failed to write free-space bitmap block " << block_number << "\n";
            return false;
        }
    }
    return true;
}


bool FileSystem::add_directory_entry(int directory_inode_index, int inode_index, const std::string& name) {
    // Out of bounds check
    if (directory_inode_index < 0 || directory_inode_index >= m_max_inodes) {
        std::cerr << "add_dir_entry: invalid dir inode index\n";
        return false;
    }
    // Check if Inode index belongs to directory
    Inode& directory_inode = m_inode_table[directory_inode_index];
    if (directory_inode.type != InodeType::DIRECTORY) {
        std::cerr << "add_dir_entry: inode is not a directory\n";
        return false;
    }

    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);
    
    // Gets block of directory
    if (!m_disk.read_block(directory_inode.index_block, buffer.data())) {
        std::cerr << "add_dir_entry: failed to read directory block\n";
        return false;
    }

    DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(DirectoryEntry));
    int free_slot = -1;

    // Find free slot
    for (int i = 0; i < max_entries; ++i) {
        if (entries[i].inode_index == -1) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) {
        std::cerr << "Directory is full (single-block directory)\n";
        return false;
    }
    
    // Add directory
    DirectoryEntry& e = entries[free_slot];
    e.inode_index = inode_index;
    std::memset(e.name, 0, sizeof(e.name));
    std::strncpy(e.name, name.c_str(), sizeof(e.name) - 1);

    if (!m_disk.write_block(directory_inode.index_block, buffer.data())) {
        std::cerr << "add_dir_entry: failed to write directory block\n";
        return false;
    }

    directory_inode.size += 1; // interpret as "entry count"
    return true;
}

int FileSystem::find_directory_entry(int directory_inode_index, const std::string& name) {
    if (directory_inode_index < 0 || directory_inode_index>= m_max_inodes) {
        return -1;
    }

    Inode& dir_inode = m_inode_table[directory_inode_index];
    if (dir_inode.type != InodeType::DIRECTORY) {
        return -1;
    }

    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);

    if (!m_disk.read_block(dir_inode.index_block, buffer.data())) {
        std::cerr << "find_dir_entry: failed to read directory block\n";
        return -1;
    }

    DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(buffer.data());
    int max_entries   = block_size / static_cast<int>(sizeof(DirectoryEntry));

    for (int i = 0; i < max_entries; ++i) {
        if (entries[i].inode_index != -1) {
            if (std::strncmp(entries[i].name, name.c_str(), sizeof(entries[i].name)) == 0) {
                return entries[i].inode_index;
            }
        }
    }

    return -1; // not found
}

std::vector<std::string> FileSystem::split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;

    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

int FileSystem::resolve_path(const std::string& path) {
    auto parts = split_path(path);

    // "/" or "" is root
    int current_inode = m_superblock.root_inode_index;
    if (parts.empty()) {
        std::cout << "CURRNET INODE IS ROOT\n";
        return current_inode;
    }

    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& name = parts[i];

        int next_inode = find_directory_entry(current_inode, name);
        if (next_inode < 0) {
            return -1; // not found
        }

        // For intermediate components, must be directory
        if (i + 1 < parts.size()) {
            if (m_inode_table[next_inode].type != InodeType::DIRECTORY) {
                return -1; // cannot traverse through a file
            }
        }
        current_inode = next_inode;
    }
    return current_inode;
}

int FileSystem::resolve_parent_directory(const std::string& path, std::string& leaf) {
    auto parts = split_path(path);
    if (parts.empty()) {
        // no component -> no parent (invalid for mkdir/create_file)
        return -1;
    }

    leaf = parts.back();
    parts.pop_back(); // remaining are the parent path components

    int current_inode = m_superblock.root_inode_index;

    if (parts.empty()) {
        // parent is root, leaf is just the name
        return current_inode;
    }

    for (const auto& name : parts) {
        int next_inode = find_directory_entry(current_inode, name);
        if (next_inode < 0) {
            return -1; // parent component not found
        }
        if (m_inode_table[next_inode].type != InodeType::DIRECTORY) {
            return -1; // not a directory in the path
        }
        current_inode = next_inode;
    }

    return current_inode;
}



bool FileSystem::create_directory(const std::string& path) {
    std::string leaf;
    int parent_inode = resolve_parent_directory(path, leaf);
    if (parent_inode < 0) {
        std::cerr << "mkdir: parent directory does not exist for path " << path << "\n";
        return false;
    }

    // Prevent duplicates
    if (find_directory_entry(parent_inode, leaf) != -1) {
        std::cerr << "mkdir: entry already exists: " << leaf << "\n";
        return false;
    }

    int inode_index = allocate_inode();
    if (inode_index < 0) {
        return false;
    }

    int block = allocate_block();
    if (block < 0) {
        std::cerr << "mkdir: failed to allocate data block\n";
        return false;
    }

    // Setup new directory inode
    Inode& dir_inode = m_inode_table[inode_index];
    dir_inode.type        = InodeType::DIRECTORY;
    dir_inode.index_block = block;
    dir_inode.size        = 0;

    // Initialize its directory block
    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);
    DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(DirectoryEntry));
    for (int i = 0; i < max_entries; ++i) {
        entries[i].inode_index = -1;
        std::memset(entries[i].name, 0, sizeof(entries[i].name));
    }

    if (!m_disk.write_block(block, buffer.data())) {
        std::cerr << "mkdir: failed to write directory block\n";
        return false;
    }

    if (!write_inode_table_to_disk()) {
        std::cerr << "mkdir: failed to persist inode table\n";
        return false;
    }

    if (!add_directory_entry(parent_inode, inode_index, leaf)) {
        std::cerr << "mkdir: failed to add dir entry to parent\n";
        return false;
    }

    return true;
}

bool FileSystem::create_file(const std::string& path) {
    std::string leaf;
    int parent_inode = resolve_parent_directory(path, leaf);
    if (parent_inode < 0) {
        std::cerr << "create_file: parent directory does not exist for path " << path << "\n";
        return false;
    }

    // File/dir already exists?
    if (find_directory_entry(parent_inode, leaf) != -1) {
        std::cerr << "create_file: entry already exists: " << leaf << "\n";
        return false;
    }

    int inode_index = allocate_inode();
    if (inode_index < 0) {
        return false;
    }

    int index_block = allocate_block();
    if (index_block < 0) {
        std::cerr << "create_file: failed to allocate index block\n";
        return false;
    }

    Inode& inode = m_inode_table[inode_index];
    inode.type = InodeType::FILE;
    inode.index_block = index_block;
    inode.size = 0;

    // Initialize index block: all entries = -1
    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);
    int* entries = reinterpret_cast<int*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(int));
    for (int i = 0; i < max_entries; ++i) {
        entries[i] = -1;
    }
    if (!m_disk.write_block(index_block, buffer.data())) {
        std::cerr << "create_file: failed to write index block\n";
        return false;
    }

    if (!write_inode_table_to_disk()) {
        std::cerr << "create_file: failed to persist inode table\n";
        return false;
    }

    if (!add_directory_entry(parent_inode, inode_index, leaf)) {
        std::cerr << "create_file: failed to add dir entry to parent\n";
        return false;
    }

    return true;
}

bool FileSystem::write_file(int file_index, const std::string& data) {
    if (file_index < 0 || file_index >= static_cast<int>(m_open_files.size())) {
        std:: cerr <<"out of bounds file index\n";
    }

    OpenFileEntry& entry {m_open_files[file_index]};
    if (!entry.in_use) {
        std::cerr << "file not open\n";
        return false;
    }

    Inode& inode = m_inode_table[entry.inode_index];
    if (inode.type != InodeType::FILE) {
        std::cerr << "write_file: not a regular file:\n";
        return false;
    }

    if (inode.index_block < 0) {
        std::cerr << "write_file: inode has no index block\n";
        return false;
    }

    const int block_size = m_disk.block_size();

    std::vector<char> idx_buf(block_size, 0);
    if (!m_disk.read_block(inode.index_block, idx_buf.data())) {
        std::cerr << "write_file: failed to read index block\n";
        return false;
    }

    int* entries = reinterpret_cast<int*>(idx_buf.data());
    int max_entries = block_size / static_cast<int>(sizeof(int));

    int data_len = static_cast<int>(data.size());
    int max_bytes = static_cast<int>(max_entries) * block_size;
    if (data_len > max_bytes) {
        std::cerr << "write_file: data too large, truncating to " << max_bytes << " bytes\n";
        data_len = max_bytes;
    }

    int num_blocks_needed = static_cast<int>((data_len + block_size - 1) / block_size);

    int offset = 0;
    for (int i = 0; i < num_blocks_needed; ++i) {
        if (entries[i] == -1) {
            int new_block = allocate_block();
            if (new_block < 0) {
                std::cerr << "write_file: out of blocks\n";
                return false;
            }
            entries[i] = new_block;
        }

        int block_number = entries[i];

        std::vector<char> data_buf(block_size, 0);
        int64_t remaining = data_len - offset;
        int bytes_this_block = static_cast<int>(std::min<int64_t>(remaining, block_size));

        std::memcpy(data_buf.data(), data.data() + offset, bytes_this_block);

        if (!m_disk.write_block(block_number, data_buf.data())) {
            std::cerr << "write_file: disk write failed\n";
            return false;
        }
        offset += bytes_this_block;
    }


    // Write updated index block back
    if (!m_disk.write_block(inode.index_block, idx_buf.data())) {
        std::cerr << "write_file: failed to write index block\n";
        return false;
    }

    inode.size = static_cast<int>(data_len);

    if (!write_inode_table_to_disk()) {
        std::cerr << "write_file: failed to persist inode table\n";
        return false;
    }

    return true;
}

bool FileSystem::read_file(int file_index, std::string& out) {
    if (file_index < 0 || file_index >= static_cast<int>(m_open_files.size())) {
        std:: cerr <<"out of bounds file index\n";
    }

    OpenFileEntry& entry {m_open_files[file_index]};
    if (!entry.in_use) {
        std::cerr << "file not open\n";
        return false;
    }

    const Inode& inode = m_inode_table[entry.inode_index];
    if (inode.type != InodeType::FILE) {
        std::cerr << "read_file: not a regular file\n";
        return false;
    }

    if (inode.index_block < 0) {
        std::cerr << "read_file: inode has no index block\n";
        return false;
    }

    const int block_size = m_disk.block_size();

    // Load index block
    std::vector<char> idx_buf(block_size, 0);
    if (!m_disk.read_block(inode.index_block, idx_buf.data())) {
        std::cerr << "read_file: failed to read index block\n";
        return false;
    }

    const int* entries = reinterpret_cast<const int*>(idx_buf.data());
    int max_entries = block_size / static_cast<int>(sizeof(int));

    int64_t remaining = inode.size;
    out.clear();
    out.reserve(inode.size);
    for (int i = 0; i < max_entries && remaining > 0; ++i) {
        int block_no = entries[i];
        if (block_no == -1)
            break; // no more blocks

        std::vector<char> data_buf(block_size, 0);
        if (!m_disk.read_block(block_no, data_buf.data())) {
            std::cerr << "read_file: disk read failed\n";
            return false;
        }

        int bytes_this_block = static_cast<int>(std::min<int64_t>(remaining, block_size));
        out.append(data_buf.data(), data_buf.data() + bytes_this_block);
        remaining -= bytes_this_block;
    }
    return true;
}

int FileSystem::open_file(const std::string& path) {
    int inode_index = resolve_path(path);
    if (inode_index < 0) {
        std::cerr << "open_file: path not found: " << path << "\n";
        return -1;
    } 

    Inode& inode = m_inode_table[inode_index];
    if (inode.type != InodeType::FILE) {
        std::cerr << "open: not a regular file: " << path << "\n";
        return -1;
    }
    int file_index = -1;
    int m_open_files_size = static_cast<int>(m_open_files.size());
    for (int i = 0; i < m_open_files_size; ++i) {
        if (!m_open_files[i].in_use) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        file_index = m_open_files_size;
        m_open_files.push_back(OpenFileEntry{});
    }

    OpenFileEntry& entry = m_open_files[file_index];
    entry.inode_index = inode_index;
    entry.offset = 0;
    entry.in_use = true;

    return file_index;
}

bool FileSystem::close_file(int file_index) {
    if (file_index < 0 || file_index >= static_cast<int>(m_open_files.size())) {
        std::cerr << "close: invalid fd\n";
        return false;
    }
    OpenFileEntry& entry = m_open_files[file_index];
    if (!entry.in_use) {
        std::cerr << "close: fd not in use\n";
        return false;
    }

    entry.in_use = false;
    entry.inode_index = -1;
    entry.offset = 0;
    return true;
}

void FileSystem::recursive_search(int directory_inode_index, const std::string& dir_path, const std::string& pattern, std::vector<std::string>& results) {
    if (directory_inode_index < 0 || directory_inode_index >= m_max_inodes) {
        return;
    } 

    const Inode& dir_inode = m_inode_table[directory_inode_index];
    if (dir_inode.type != InodeType::DIRECTORY) {
        return;
    } 

    int dir_block = dir_inode.index_block;
    if (dir_block < 0) {
        return;
    }

    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);

    if (!m_disk.read_block(dir_block, buffer.data())) {
        std::cerr << "search: failed to read directory block at " << dir_block << "\n";
        return;
    }
    
    const DirectoryEntry* entries = reinterpret_cast<const DirectoryEntry*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(DirectoryEntry));
    for (int i = 0; i < max_entries; ++i) {
        const DirectoryEntry& e = entries[i];

        if (e.inode_index == -1) {
            continue; // unused slot
        }

        // Build std::string from fixed-size char array
        std::string name {e.name};
        if (name.empty()) {
            continue;
        }

        int child_inode_index = e.inode_index;
        if (child_inode_index < 0 || child_inode_index >= m_max_inodes) {
            continue;
        }

        const Inode& child_inode = m_inode_table[child_inode_index];

        // Build full path: handle root specially to avoid "//"
        std::string child_path;
        if (dir_path == "/") {
            child_path = "/" + name;
        } else {
            child_path = dir_path + "/" + name;
        }

        if (name.find(pattern) != std::string::npos) {
            results.push_back(child_path);
        }

        if (child_inode.type == InodeType::DIRECTORY) {
            recursive_search(child_inode_index, child_path, pattern, results);
        }
    }
}

std::vector<std::string> FileSystem::search(const std::string& pattern) {
    std::vector<std::string> results;

    int root_inode = m_superblock.root_inode_index;
    if (root_inode < 0 || root_inode >= m_max_inodes) {
        std::cerr << "search: invalid root inode index\n";
        return results;
    }

    recursive_search(root_inode, "/", pattern, results);
    return results;
}

bool FileSystem::list_directory_entries(const std::string& path, std::vector<DirectoryEntry>& out) {
    out.clear();

    int inode_index = resolve_path(path);
    if (inode_index < 0 || inode_index >= m_max_inodes) {
        std::cerr << "list_directory_entries: path not found: " << path << "\n";
        return false;
    }
    const Inode& dir_inode = m_inode_table[inode_index];
    std::cout << "INODE INDEX: " << inode_index;
    std::cout << "inode type (raw): " << static_cast<int>(dir_inode.type) << "\n";
    if (dir_inode.type != InodeType::DIRECTORY) {
        std::cerr << "list_directory_entries: not a directory: " << path << "\n";
        return false;
    }


    int dir_block = dir_inode.index_block;
    if (dir_block < 0) {
        std::cerr << "list_directory_entries: directory has no block\n";
        return false;
    }

    const int block_size = m_disk.block_size();
    std::vector<char> buffer(block_size, 0);

    if (!m_disk.read_block(dir_block, buffer.data())) {
        std::cerr << "list_directory_entries: failed to read directory block\n";
        return false;
    }

    const DirectoryEntry* entries = reinterpret_cast<const DirectoryEntry*>(buffer.data());
    int max_entries = block_size / static_cast<int>(sizeof(DirectoryEntry));

    for (int i = 0; i < max_entries; ++i) {
        if (entries[i].inode_index == -1) {
            continue;
        }
        if (entries[i].name[0] == '\0') {
            continue;
        }
        out.push_back(entries[i]);
    }
    return true;
}

bool FileSystem::is_directory_inode(int inode_index) {
    if (inode_index < 0 || inode_index >= m_max_inodes) {
        return false;
    }
    return m_inode_table[inode_index].type == InodeType::DIRECTORY;
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

    if (m_superblock.id != SUPERBLOCK_MAGIC) {
        std::cerr << "mount: invalid superblock magic, need initialize\n";
        return false;
    }

    if (!FileSystem::read_inode_table_from_disk()) {
        std::cerr <<"Reading inode table from disk failed\n";
        return false;
    }

    if (!FileSystem::read_free_bitmap_from_disk()) {
        std::cerr << "Reading free bitmap from disk failed\n";
        return false;
    }
    return true;
}


