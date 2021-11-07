#include "file-tree.h"

#include "core/core.h"
#include "katla/core/posix-file.h"

#include "backer.h"

#include <filesystem>
#include <openssl/md5.h>
#include <optional>

namespace backer {

    namespace fs = std::filesystem;

    FileTree::FileTree() {
    }

    std::shared_ptr<FileSystemEntry> FileTree::create(std::string path) {
        return fileSystemEntryFromDir(fs::directory_entry(fs::path(path)), path, {});
    }

    std::vector<std::shared_ptr<backer::FileSystemEntry>> FileTree::flatten(const std::shared_ptr<FileSystemEntry>& entry)
    {
        if(!entry) {
            return {};
        }

        std::vector<std::shared_ptr<FileSystemEntry>> files;
        files.push_back(entry);

        flattenChild(*entry, files);
        return files;
    }

    void FileTree::flattenChild(const FileSystemEntry& entry, std::vector<std::shared_ptr<backer::FileSystemEntry>>& list)
    {
        if (entry.children.has_value()) {
            for(auto& file : entry.children.value()) {
                list.push_back(file);

                if (file->type == FileSystemEntryType::Dir) {
                    flattenChild(*file, list);
                }
            }
        }
    }

    std::unique_ptr<FileSystemEntry> FileTree::fileSystemEntryFromFile(const std::filesystem::directory_entry& entry, std::string basePath, std::optional<std::weak_ptr<FileSystemEntry>> parent)
    {
        if (!entry.is_regular_file()) {
            return {};
        }

        auto fileData = std::make_unique<FileSystemEntry>();
        fileData->name = entry.path().filename().string();
        fileData->relativePath = fs::relative(entry.path(), basePath).string();

        auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
        if (!absolutePathResult) {
            throw std::runtime_error(absolutePathResult.error().message());
        }

        fileData->absolutePath = absolutePathResult.value();
        fileData->size = entry.file_size();
        fileData->type = FileSystemEntryType::File;

        if (parent.has_value()) {
            fileData->parent = parent.value();
        }

        return fileData;
    }

    std::shared_ptr<FileSystemEntry> FileTree::fileSystemEntryFromDir(const std::filesystem::directory_entry& entry, std::string basePath, std::optional<std::weak_ptr<FileSystemEntry>> parent)
    {
        if (!entry.is_directory()) {
            return {};
        }

        auto dirData = std::make_shared<FileSystemEntry>();
        dirData->type = backer::FileSystemEntryType::Dir;
        dirData->name = entry.path().filename().string();
        dirData->relativePath = fs::relative(entry.path(), basePath).string();

        auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
        if (!absolutePathResult) {
            throw std::runtime_error(absolutePathResult.error().message());
        }

        dirData->absolutePath = absolutePathResult.value();

        if (parent.has_value()) {
            dirData->parent = parent.value();
        }

        // TODO: ignore node_modules and .git for now
        std::size_t found = dirData->absolutePath.find("node_modules");
        if (found != std::string::npos) {
            return {};
        }
        found = dirData->absolutePath.find(".git");
        if (found != std::string::npos) {
            return {};
        }
        //--

        fs::directory_iterator dirIter(entry.path());

        dirData->children = std::vector<std::shared_ptr<FileSystemEntry>>();

        size_t dirSize = 0;
        for (auto &entry : dirIter) {
            if (entry.is_symlink() || entry.is_other()) {
                continue;
            }

            if (entry.is_regular_file() ) {
                auto childFileData = fileSystemEntryFromFile(entry, basePath, dirData);
                dirSize += childFileData->size;
                dirData->children->push_back(std::move(childFileData));
            }
            if (entry.is_directory()) {
                auto childDirData = fileSystemEntryFromDir(entry, basePath, dirData);
                if (!childDirData) {
                    continue;
                }

                dirSize += childDirData->size;
                dirData->children->push_back(std::move(childDirData));
            }      
        }

        dirData->size = dirSize;

        return dirData;
    }

    void FileTree::fillDataFromIndex(FileSystemEntry& entry, const std::vector<FileSystemEntry>& index)
    {
        auto basePath = entry.absolutePath;

        // convert index to a map using relativePath
        std::map<std::string, FileSystemEntry> indexMap;
        for(const auto& it : index) {
            indexMap[it.relativePath] = it;
        } 

        fillDataFromIndexRecursive(entry, indexMap);
    }

    void FileTree::fillDataFromIndexRecursive(FileSystemEntry& entry, const std::map<std::string, FileSystemEntry>& index)
    {
        auto findIt = index.find(entry.relativePath);
        if (findIt != index.end()) {
            entry.hash = findIt->second.hash;
            entry.size = findIt->second.size;
        }

        if (entry.type == FileSystemEntryType::File) {   
            return;
        }

        if (entry.children.has_value()) {
            for(auto& childEntry : entry.children.value()) {
                fillDataFromIndexRecursive(*childEntry, index);
            }
        }
    }

    void FileTree::recursiveHash(FileSystemEntry& entry) {

        if (entry.type == FileSystemEntryType::File) {
            if (!entry.hash) {
                auto sha256 = Backer::sha256(entry.absolutePath);
                entry.hash = std::move(sha256);
            }
            return;
        }

        std::vector<std::vector<std::byte>> dirHashes;
        if (entry.children.has_value()) {

            // order of children is important, must be sorted
            std::map<std::string, std::shared_ptr<FileSystemEntry>> sortedChildren;
            for(auto& childEntry : entry.children.value()) {
                sortedChildren[childEntry->relativePath] = childEntry;
            }

            for(auto& childEntry : sortedChildren) {
                recursiveHash(*childEntry.second);

                if (childEntry.second->hash.has_value() && childEntry.second->hash.value().size()) {
                    dirHashes.push_back(childEntry.second->hash.value());
                }
            }
        }

        // also calculate empty hash for empty dirs
        if (!entry.hash.has_value()) {
            entry.hash = backer::Backer::sha256(dirHashes);
        }
    }

} // namespace backer