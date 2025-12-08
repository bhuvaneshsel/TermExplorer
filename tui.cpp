#include "disk.hpp"
#include "tui.hpp"


#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>
#include <algorithm>

static FileNode* find_node_by_path(FileNode& node, const std::string& path) {
    if (node.path == path)
        return &node;

    for (auto& child : node.children) {
        if (auto* found = find_node_by_path(*child, path)) {
            return found;
        }
    }
    return nullptr;
}

static bool create_file_at_selection(std::shared_ptr<AppState> state) {
    if (state->new_file_name.empty())
        return false;

    std::vector<FileNode*> visible;
    build_visible_file_tree(state->fs, state->root, visible);
    if (visible.empty())
        return false;

    if (state->selected_index >= static_cast<int>(visible.size())) {
        state->selected_index = static_cast<int>(visible.size() - 1);
    }

    FileNode* selected = visible[state->selected_index];

    // Determine the directory to create the file in
    FileNode* dir_node = nullptr;
    std::string dir_path;

    if (selected->is_directory) {
        dir_node = selected;
        dir_path = selected->path;
    } else {
        // Parent directory of selected file
        std::string path = selected->path;
        auto pos = path.find_last_of('/');
        if (pos == std::string::npos || pos == 0) {
            dir_path = "/";
        } else {
            dir_path = path.substr(0, pos);
        }
        dir_node = find_node_by_path(state->root, dir_path);
    }

    if (!dir_node)
        return false;

    // Build full file path
    std::string full_path;
    if (dir_path == "/")
        full_path = "/" + state->new_file_name;
    else
        full_path = dir_path + "/" + state->new_file_name;

    if (!state->fs.create_file(full_path))
        return false;

    // Invalidate children so this directory gets reloaded next time
    dir_node->children.clear();
    return true;
}

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

    auto tree = ftxui::vbox(std::move(lines)) | ftxui::border;
    
    if (!state->creating_file) {
        return tree;
    }

    // Overlay popup on top of tree
    return ftxui::dbox({
        tree,
        render_input_popup(state),
    });
}

bool handle_event(ftxui::Event e, ftxui::ScreenInteractive& screen, std::shared_ptr<AppState> state) {

    if (state->creating_file) {
        // Cancel popup
        if (e == ftxui::Event::Escape) {
            state->creating_file = false;
            state->new_file_name.clear();
            return true;
        }

        // Submit popup
        if (e == ftxui::Event::Return) {
            create_file_at_selection(state);   
            state->creating_file = false;
            state->new_file_name.clear();
            return true;
        }

        return state->input_box->OnEvent(e);
    }

    std::vector<FileNode*> visible;
    build_visible_file_tree(state->fs, state->root, visible);

    int n = static_cast<int>(visible.size());

    if (n == 0) {
        return false;
    }

    if (state->selected_index >= n) {
        state->selected_index = n - 1;
    }

    if (e == ftxui::Event::Character('n')) {
        state->creating_file = true;
        state->new_file_name.clear();
        return true;
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

ftxui::Element render_input_popup(std::shared_ptr<AppState> state) {

    return ftxui::window(
        ftxui::text(" Enter file name ") | ftxui::bold,
        ftxui::vbox({
            state->input_box->Render() | ftxui::border,
            ftxui::text("Press Enter to confirm, Esc to cancel") | ftxui::dim,
        })
    ) | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 40)
      | ftxui::center;
}

void run_tui(FileSystem& fs) {
    FileNode root{};
    root.name = "/";
    root.path = "/";
    root.is_directory = true;
    root.is_expanded = true;
    
    auto state = std::make_shared<AppState>(AppState{fs, std::move(root), 0});
    state->input_box = ftxui::Input(&state->new_file_name, "filename");

    auto screen = ftxui::ScreenInteractive::Fullscreen();

    ftxui::Component renderer = ftxui::Renderer([state] {
        return render_tree(state);
    });

    ftxui::Component app = ftxui::CatchEvent(renderer, [&screen, state](ftxui::Event e) {
            return handle_event(e, screen, state);
    });

    screen.Loop(app);
}

