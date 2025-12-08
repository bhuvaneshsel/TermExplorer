#ifndef DISK_H
#define DISK_H

#include <string>
#include <fstream>

class Disk 
{
public:

    Disk(int number_of_blocks, int block_size);

    bool open(const std::string& path);

    void close();

    bool is_open() const { return m_file.is_open(); };

    bool read_block(int block_number, void* buffer);

    bool write_block(int block_number, const void* buffer);

    int block_size() const { return m_block_size; };

    int number_of_blocks() const { return m_number_of_blocks; };

    void flush();

private:
    std::fstream m_file{};
    std::string m_path{};
    
    const int m_number_of_blocks{};
    const int m_block_size{};

    bool ensure_size();
};
#endif
