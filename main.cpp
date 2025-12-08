#include "file_tree.hpp"
#include "tui.hpp"
#include "disk.hpp"
#include "filesystem.hpp"
#include <iostream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;


int main() {
    // FileTree tree{"."};
    //
    // FileNode& root = tree.get_root_node();
    // root.is_expanded = true;
    //
    // run_tui(tree);

    int number_of_blocks {1024};
    int block_size = {4096};
    Disk disk { number_of_blocks, block_size };

    disk.open("disk.img");

    FileSystem fs{disk, 64};
    fs.initialize();
    disk.close();

    disk.open("disk.img");
    fs.mount();
    const auto& sb = fs.superblock();
    std::cout << "total_blocks:      " << sb.total_blocks << "\n";
    std::cout << "block_size:        " << sb.block_size << "\n";
    std::cout << "inode_table_start: " << sb.inode_table_start << "\n";

    const auto& inodes = fs.inode_table();
    if (!inodes.empty()) {
        std::cout << "root inode type:   "
                  << (inodes[0].type == InodeType::DIRECTORY ? "DIRECTORY" : "OTHER")
                  << "\n";
        std::cout << "root index_block:  " << inodes[0].index_block << "\n";
    }
    disk.close();
    return 0;
}
