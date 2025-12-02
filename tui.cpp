
#include "file_tree.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

struct AppState {
    FileTree& tree;
    int selected_index = 0;
};

static Element render_tree(const AppState& state, const std::vector<const FileNode*>& visible) {
    Elements lines;

    for (int i = 0; i < (int)visible.size(); ++i) {
        const FileNode* node = visible[i];

        std::string label {};
        label.append(node->depth() * 4, ' ');
        if (node->is_directory) {
            label += node->is_expanded ? " " : " ";
        } else {
            label += " ";
        }
        label += node->name();

        Element row = text(label);
        if (i == state.selected_index) {
            row = row | inverted;
        }
        lines.push_back(row);
    }

    return vbox(std::move(lines)) | border;
}

static bool handle_event(AppState& state, const std::vector<const FileNode*>& visible, Event event, ScreenInteractive& screen, int visible_nodes_size) {

    if (event == Event::Character('q')) {
        screen.Exit();
        return true;
    }

    if (event == Event::ArrowDown) {
        if (state.selected_index + 1 < visible_nodes_size)
            state.selected_index++;
        return true;
    }

    if (event == Event::ArrowUp) {
        if (state.selected_index > 0)
            state.selected_index--;
        return true;
    }

    if (event == Event::Return) {
        FileNode* node = const_cast<FileNode*>(visible[state.selected_index]);
        if (node->is_directory) {
            node->is_expanded = !node->is_expanded;
        }
        return true;
    }

    return false;
}

void static clamp_selection(AppState& state, int visible_nodes_size) {
    if (visible_nodes_size == 0) {
        state.selected_index = 0;
        return;
    }
    if (state.selected_index >= visible_nodes_size) {
        state.selected_index = visible_nodes_size - 1;
    }
}

void run_tui(FileTree& tree) {
    AppState state{ tree, 0 };
    auto screen = ScreenInteractive::Fullscreen();

    Component renderer = Renderer([&] {
        std::vector<const FileNode*> visible_nodes = tree.get_visible_nodes();
        int visible_nodes_size = static_cast<int>(visible_nodes.size());
        
        clamp_selection(state, visible_nodes_size);
        return render_tree(state, visible_nodes);
    });

    renderer = CatchEvent(renderer, [&](Event event) {
        std::vector<const FileNode*> visible_nodes = tree.get_visible_nodes();
        int visible_nodes_size = static_cast<int>(visible_nodes.size());
        return handle_event(state, visible_nodes, event, screen, visible_nodes_size);
    });

    screen.Loop(renderer);
}
