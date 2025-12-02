#ifndef FILE_NODE_H
#define FILE_NODE_H

#include <filesystem>
#include <vector>
#include <string>

struct FileNode {
    std::filesystem::path path {};
    bool is_directory { false };
    bool is_expanded { false };

    FileNode* parent { nullptr };
    std::vector<FileNode> children {};
    
    std::string name() const {
        return path.filename().string();
    }

    bool is_leaf() const {
        return children.empty();
    }

    int depth() const {
        int depth = 0;
        const FileNode* cur { parent };
        while (cur) {
            cur = cur->parent;
            depth++;
        }
        return depth;
    }
};

#endif
