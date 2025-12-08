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

    // Format the filesystem the first time
    // fs.initialize();

    // Mount it (loads inode table + bitmap)
    fs.mount();

    // Create directory and file
    // fs.create_directory("/big");
    // fs.create_file("/big/test.txt");

    // Generate 3000 bytes of test data
    std::string long_text = generate_test_data();

    // Write the file (should allocate ~6 blocks)
    // if (!fs.write_file("/big/test.txt", long_text)) {
    //     std::cerr << "write_file failed\n";
    //     return 1;
    // }

    // std::cout << "Wrote " << long_text.size() << " bytes to /big/test.txt\n";

    // Now read it back
    std::string out;
    if (!fs.read_file("/big/test.txt", out)) {
        std::cerr << "read_file failed\n";
        return 1;
    }

    std::cout << "Read back " << out.size() << " bytes\n";

    std::cout << out << "\n";
    // Verify correctness
    if (out == long_text) {
        std::cout << "SUCCESS: file contents match\n";
    } else {
        std::cout << "ERROR: file mismatch!\n";
    }
}
