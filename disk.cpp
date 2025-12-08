#include "disk.hpp"

#include <filesystem>
#include <iostream>

Disk::Disk(int number_of_blocks, int block_size) : m_number_of_blocks{number_of_blocks}, m_block_size{block_size} {};

bool Disk::open(const std::string& path)
{
    m_path = path;
    bool exists = std::filesystem::exists(path);

    std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;
    if (!exists) {
         mode |= std::ios::trunc;
    }
    
    m_file.open(path, mode);
    if (!m_file.is_open())
    {
        std::cerr << "Failed to open disk at path " << path << "\n";
        return false;
    }

    if (!ensure_size())
    {
        std::cerr << "Could not allocate size for disk\n";
        m_file.close();
        return false;
    }
    return true;

}

bool Disk::ensure_size()
{
    const std::uintmax_t desired_size = static_cast<std::uintmax_t>(m_number_of_blocks * m_block_size);

    std::uintmax_t current_size {};

    if (std::filesystem::exists(m_path)) {
        current_size = std::filesystem::file_size(m_path);
        std::cout << "Current disk size: " << current_size << "\n";
    }

    if (current_size == desired_size) {
        return true;
    }

    // if (current_size > desired_size) {
    //     std::cerr << "Disk size is larger than desired size\n";
    //     return false;
    // }

    m_file.seekp(0, std::ios::end);
    std::vector<char> zeros(m_block_size, 0);

    while (current_size < desired_size) {
        std::uintmax_t remaining = desired_size - current_size;
        std::streamsize chunk = static_cast<std::streamsize>(std::min<std::uintmax_t>(remaining, zeros.size()));
        m_file.write(zeros.data(), chunk);
        if (!m_file) {
            std::cerr << "Failed to extend disk size\n";
            return false;
        }
        current_size += static_cast<std::uintmax_t>(chunk);
    }

    m_file.flush();
    std::cout << "Disk size after intitialization: " << std::filesystem::file_size(m_path) << "\n";

    return true;
}

bool Disk::read_block(int block_number, void* buffer) {
    if (!m_file.is_open())
    {
        std::cerr << "Disk is not open\n";
        return false;
    }
    if (block_number >= m_number_of_blocks) {
        std::cerr << "Block number out of range for reading\n";
        return false;
    }
    std::streamoff offset = static_cast<std::streamoff>(block_number) * static_cast<std::streamoff>(m_block_size);
    m_file.seekg(offset, std::ios::beg);
    if (!m_file) {
        std::cerr << "Moving read pointer failed\n";
        return false;
    }

    m_file.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(m_block_size));

    if (!m_file) {
        std::cerr << "Read failed\n";
        return false;
    }

    return true;
}

bool Disk::write_block(int block_number, const void* buffer) {
    if (!m_file.is_open()) {
        std::cerr << "Disk is not open\n";
        return false;
    }

    if (block_number >= m_number_of_blocks) {
        std::cerr << "Block number out of range for writing\n";
        return false;
    }
    
    std::streamoff offset = static_cast<std::streamoff>(block_number) * static_cast<std::streamoff>(m_block_size);

    m_file.seekp(offset, std::ios::beg);
    if (!m_file) {
        std::cerr << "Moving disk write pointer failed\n";
        return false;
    }

    m_file.write(reinterpret_cast<const char*>(buffer), static_cast<std::streamsize>(m_block_size));

    if (!m_file) {
        std::cerr << "Write failed\n";
        return false;
    }

    return true;
}


void Disk::close() {
    if (m_file.is_open()) {
        m_file.close();
    }
}
