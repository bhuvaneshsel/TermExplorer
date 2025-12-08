#include "file_tree.hpp"
#include "tui.hpp"
#include "disk.hpp"
#include "filesystem.hpp"
#include <iostream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

std::string generate_test_data() {
    std::string data;
    data.reserve(3000);

    for (int i = 0; i < 3000; ++i) {
        data.push_back('A' + (i % 26));   // ABCDE... repeated
    }

    return data;
}

int main() {
    // FileTree tree{"."};
    //
    // FileNode& root = tree.get_root_node();
    // root.is_expanded = true;
    //
    // run_tui(tree);
    Disk disk(1024, 512);
    disk.open("disk.img");

    FileSystem fs(disk, 128);

    fs.initialize();
    fs.mount();

    fs.create_directory("/a");
    fs.create_directory("/a/b");
    fs.create_file("/a/b/cat.txt");
    fs.create_file("/a/b/dog.txt");
    fs.create_file("/a/file_cat.log");

    auto results = fs.search("cat");

    std::cout << "Search results for 'cat':\n";
    for (const auto& path : results) {
        std::cout << "  " << path << "\n";
    }
}
