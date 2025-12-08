#include "tui.hpp"
#include "disk.hpp"
#include "filesystem.hpp"
#include <iostream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>


int main() {
    Disk disk(1024, 512);
    if (!disk.open("disk.img")) {
        std::cerr << "Failed to open disk image\n";
        return 1;
    }

    FileSystem fs(disk, 128);

    // Try to mount; if it fails, initialize then mount again
    if (!fs.mount()) {
        std::cout << "No valid filesystem found. Initializing...\n";

        if (!fs.initialize()) {
            std::cerr << "Failed to initialize filesystem\n";
            return 1;
        }

        if (!fs.mount()) {
            std::cerr << "Failed to mount filesystem after initialization\n";
            return 1;
        }

        // Create some initial content once after fresh format
        fs.create_directory("/a");
        fs.create_directory("/a/b");
        fs.create_directory("/docs");
        fs.create_file("/a/b/c.txt");
        fs.create_file("/docs/readme.md");
        fs.create_file("/root_file.txt");
    }

    // Sanity check
    std::vector<DirectoryEntry> test_entries;
    if (fs.list_directory_entries("/", test_entries)) {
        std::cout << "Root entries:\n";
        for (auto& e : test_entries) {
            std::cout << "  - " << e.name << "\n";
        }
    }

    run_tui(fs);

    disk.close();
    return 0;
}
