#include "disk.hpp"
#include "tui.hpp"


#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>
#include <algorithm>


void build_visible_file_tree(FileSystem& fs, FileNode& node, std::vector<FileNode*>& out) {
    out.push_back(&node);

    if (!node.is_directory || !node.is_expanded) {
        return;
    }

    // Only load children once, the first time
    if (node.children.empty()) {
        std::vector<DirectoryEntry> entries;
        if (fs.list_directory_entries(node.path, entries)) {
            for (auto& e : entries) {
                auto child = std::make_unique<FileNode>();

                child->name = e.name;
                if (node.path == "/")
                    child->path = "/" + child->name;
                else
                    child->path = node.path + "/" + child->name;

                int child_inode = e.inode_index;
                child->is_directory = fs.is_directory_inode(child_inode);
                child->is_expanded  = false;

                node.children.push_back(std::move(child));
            }
        }
    }

    for (auto& child : node.children) {
        build_visible_file_tree(fs, *child, out);
    }
}

ftxui::Element render_tree(std::shared_ptr<AppState> state) {
    std::vector<FileNode*> visible;
    build_visible_file_tree(state->fs, state->root, visible);

    ftxui::Elements lines;
    for (int i = 0; i < (int)visible.size(); ++i) {
        FileNode* node = visible[i];

        // Compute depth from path ("/a/b" -> 2 slashes => depth 1)
        int depth = 0;
        if (node->path.size() > 1) {
            depth = (int)std::count(node->path.begin(), node->path.end(), '/') - 1;
        }

        std::string label;
        label.append(depth * 2, ' ');

        if (node->is_directory) {
            label += node->is_expanded ? " " : " ";
        } else {
            label += " ";
        }

        label += node->name;

        ftxui::Element e = ftxui::text(label);
        if (i == state->selected_index) {
            e = e | ftxui::inverted;  // highlight selected row
        }
        lines.push_back(e);
    }

    return ftxui::vbox(std::move(lines)) | ftxui::border;
}

bool handle_event(ftxui::Event e, ftxui::ScreenInteractive& screen, std::shared_ptr<AppState> state) {
    std::vector<FileNode*> visible;
    build_visible_file_tree(state->fs, state->root, visible);

    int n = static_cast<int>(visible.size());

    if (n == 0) {
        return false;
    }

    if (state->selected_index >= n) {
        state->selected_index = n - 1;
    }

    if (e == ftxui::Event::ArrowDown || e == ftxui::Event::Character('j')) {
        if (state->selected_index + 1 < n) {
            state->selected_index++;
        }
        return true;
    }

    if (e == ftxui::Event::ArrowUp || e == ftxui::Event::Character('k')) {
        if (state->selected_index > 0) {
            state->selected_index--;
        }
        return true;
    }

    FileNode* node = visible[state->selected_index];

    if (e == ftxui::Event::Return) {
        if (node->is_directory) {
            node->is_expanded = !node->is_expanded;
        }
        return true;
    }

    if (e == ftxui::Event::Character('q')) {
        screen.Exit();
        return true;
    }

    return false;
}

void run_tui(FileSystem& fs) {
    FileNode root{};
    root.name = "/";
    root.path = "/";
    root.is_directory = true;
    root.is_expanded = true;
    
    auto state = std::make_shared<AppState>(AppState{fs, std::move(root), 0});

    auto screen = ftxui::ScreenInteractive::Fullscreen();

    ftxui::Component renderer = ftxui::Renderer([state] {
        return render_tree(state);
    });

    ftxui::Component app = ftxui::CatchEvent(renderer, [&screen, state](ftxui::Event e) {
            return handle_event(e, screen, state);
    });

    screen.Loop(app);
}

