#ifndef TUI_H
#define TUI_H

#include "filesystem.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

struct FileNode {
    std::string name;
    std::string path;
    bool is_directory{};
    bool is_expanded{};
    std::vector<std::unique_ptr<FileNode>> children{};
};

struct AppState {
    FileSystem& fs;
    FileNode root;
    int selected_index{};

    bool creating_file {false};
    bool creating_directory {false};
    std::string new_file_name;
    ftxui::Component input_box;

    bool editing_file{false};
    std::string edit_content;
    std::string edit_path;
    ftxui::Component edit_box;

    bool searching{false};
    std::string search_query;
    std::vector<std::string> search_results;
    int search_selected_index{0};
    ftxui::Component search_box;
};



FileNode* find_node_by_path(FileNode& node, const std::string& path);

void build_visible_file_tree(FileSystem& fs, FileNode& node, std::vector<FileNode*>& out);

bool create_file_at_selection(std::shared_ptr<AppState> state);

bool save_edited_file(std::shared_ptr<AppState> state);

bool start_edit_file(std::shared_ptr<AppState> state, const std::string& path);

ftxui::Element render_tree(std::shared_ptr<AppState> state);

bool handle_event(ftxui::Event e, ftxui::ScreenInteractive& screen, std::shared_ptr<AppState> state);

ftxui::Element render_input_popup(std::shared_ptr<AppState> state);

ftxui::Element render_edit_popup(std::shared_ptr<AppState> state);

bool create_directory_at_selection(std::shared_ptr<AppState> state);

void perform_search(std::shared_ptr<AppState> state);

ftxui::Element render_search_panel(std::shared_ptr<AppState> state);

ftxui::Element render_status_bar(std::shared_ptr<AppState> state);

void run_tui(FileSystem& fs);

#endif
