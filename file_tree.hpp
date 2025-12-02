#ifndef FILE_TREE_H
#define FILE_TREE_H

#include "file_node.hpp"

#include <filesystem>
#include <vector>

class FileTree
{
public:
    FileTree(const std::filesystem::path& root_path);
    void print_tree() const;
    FileNode& get_root_node() { return m_root_node; };
    std::vector<const FileNode*> get_visible_nodes() const;

private:
    std::filesystem::path m_root_path {};
    FileNode m_root_node {};

    void build_tree(FileNode& node);
    void rebuild();
    void recursive_print_tree(const FileNode& node) const;
    void recursive_get_visible_nodes(const FileNode& node, std::vector<const FileNode*>& out) const;
};
#endif
