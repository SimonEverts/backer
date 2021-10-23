#include "file-tree.h"

#include "katla/core/posix-file.h"

#include "backer.h"

#include <filesystem>
#include <openssl/md5.h>

namespace backer {

    namespace fs = std::filesystem;

    FileTree::FileTree() {
    }

    std::unique_ptr<FileSystemEntry> FileTree::create(std::string path) {
        return fileSystemEntryFromDir(fs::directory_entry(fs::path(path)), path);
    }

    std::vector<std::shared_ptr<backer::FileSystemEntry>> FileTree::flatten(const std::shared_ptr<FileSystemEntry>& entry)
    {
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

    std::unique_ptr<FileSystemEntry> FileTree::fileSystemEntryFromFile(const std::filesystem::directory_entry& entry, std::string basePath)
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

        return fileData;
    }

    std::unique_ptr<FileSystemEntry> FileTree::fileSystemEntryFromDir(const std::filesystem::directory_entry& entry, std::string basePath)
    {
        if (!entry.is_directory()) {
            return {};
        }

        auto dirData = std::make_unique<FileSystemEntry>();
        dirData->type = backer::FileSystemEntryType::Dir;
        dirData->name = entry.path().filename().string();
        dirData->relativePath = fs::relative(entry.path(), basePath).string();
        katla::printInfo("{}, {}", dirData->relativePath, dirData->type);

        auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
        if (!absolutePathResult) {
            throw std::runtime_error(absolutePathResult.error().message());
        }

        dirData->absolutePath = absolutePathResult.value();

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
                auto childFileData = fileSystemEntryFromFile(entry, basePath);
                dirSize += childFileData->size;
                dirData->children->push_back(std::move(childFileData));
            }
            if (entry.is_directory()) {
                auto childDirData = fileSystemEntryFromDir(entry, basePath);
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

    // TODO test
    void FileTree::recursiveHash(FileSystemEntry& entry) {
        std::vector<std::vector<std::byte>> dirHashes;

        if (entry.type == FileSystemEntryType::File) {
            auto sha256 = Backer::sha256(entry.absolutePath);
            entry.hash = std::move(sha256);
            return;
        }

        if (entry.children.has_value()) {
            for(auto& childEntry : entry.children.value()) {

                if (childEntry->type == FileSystemEntryType::Dir) {
                    recursiveHash(*childEntry);
                }

                if (childEntry->type == FileSystemEntryType::File) {
                    auto sha256 = Backer::sha256(entry.absolutePath);
                    childEntry->hash = std::move(sha256); // childEntry.hash?
                }

                if (childEntry->hash.size()) {
                    dirHashes.push_back(childEntry->hash);
                }
            }
        }

        // also calculate empty hash for empty dirs
        entry.hash = backer::Backer::sha256(dirHashes);
    }

} // namespace backer