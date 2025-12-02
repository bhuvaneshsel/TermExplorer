#include "file_tree.hpp"

#include <filesystem>
#include <iostream>


FileTree::FileTree(const std::filesystem::path& root_path)
    : m_root_path {root_path}
{
    rebuild();
}


void FileTree::recursive_get_visible_nodes(const FileNode& node, std::vector<const FileNode*>& out) const {
    for (const auto& child : node.children) {
        out.push_back(&child);
        if (child.is_directory && child.is_expanded) {
            recursive_get_visible_nodes(child, out);
        }
    }
}

std::vector<const FileNode*> FileTree::get_visible_nodes() const {
    std::vector<const FileNode*> out {};
    recursive_get_visible_nodes(m_root_node, out);
    return out;
}

void FileTree::rebuild() {
    m_root_node.path = m_root_path;
    m_root_node.is_directory = std::filesystem::is_directory(m_root_path);
    m_root_node.is_expanded = true;
    m_root_node.parent = nullptr;
    m_root_node.children.clear();

    if (m_root_node.is_directory) {
        build_tree(m_root_node);
    }
}

void FileTree::build_tree(FileNode& node) 
{
    if (!node.is_directory)
    {
        node.children.clear();
        return;
    }

    node.children.clear();

    std::filesystem::directory_iterator di {node.path};

    for (const auto& dirEntry : di) {
        FileNode child;
        child.path = dirEntry.path();
        child.is_directory = dirEntry.is_directory();
        child.is_expanded = false;
        child.parent = &node;

        node.children.push_back(std::move(child));
    }

    for (auto& child : node.children) {
        if (child.is_directory) {
            build_tree(child);
        }
    }
}

void FileTree::recursive_print_tree(const FileNode& node) const {
    for (int i = 0; i < node.depth(); ++i) {
        std::cout << "  ";
    }

    if (node.is_directory) {
        std::cout << "󰉋 ";
    }
    else {
        std::cout << " "; 
    }

    std::cout << node.name() << "\n";

    for (const auto& child : node.children) {
        recursive_print_tree(child);
    }
}

void FileTree::print_tree() const {
    recursive_print_tree(m_root_node);
}
