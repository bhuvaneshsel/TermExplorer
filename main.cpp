#include "file_tree.hpp"
#include "tui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;


int main() {
    FileTree tree{"."};

    FileNode& root = tree.get_root_node();
    root.is_expanded = true;

    run_tui(tree);
    return 0;
}
