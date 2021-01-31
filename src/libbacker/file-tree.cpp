#include "file-tree.h"

#include "katla/core/posix-file.h"

#include <filesystem>
#include <openssl/md5.h>

namespace backer {

    namespace fs = std::filesystem;

    FileTree::FileTree() {
    }

    FileSystemEntry FileTree::create(std::string path) {
        return fileSystemEntryFromDir(fs::directory_entry(fs::path(path)), path);
    }

    std::vector<backer::FileSystemEntry> FileTree::flatten(const FileSystemEntry& entry)
    {
        std::vector<FileSystemEntry> files;
        files.push_back(entry);

        flattenChild(entry, files);
        return files;
    }

    void FileTree::flattenChild(const FileSystemEntry& entry, std::vector<backer::FileSystemEntry>& list)
    {
        if (entry.children.has_value()) {
            for(auto& file : entry.children.value()) {
                list.push_back(file);

                if (file.type == FileSystemEntryType::Dir) {
                    flattenChild(file, list);
                }
            }
        }
    }

    FileSystemEntry FileTree::fileSystemEntryFromFile(const std::filesystem::directory_entry& entry, std::string basePath)
    {
        if (!entry.is_regular_file()) {
            return {};
        }

        FileSystemEntry fileData{};
        fileData.name = entry.path().filename().string();
        fileData.relativePath = fs::relative(entry.path(), basePath).string();

        auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
        if (!absolutePathResult) {
            throw std::runtime_error(absolutePathResult.error().message());
        }

        fileData.absolutePath = absolutePathResult.value();
        fileData.size = entry.file_size();
        fileData.type = FileSystemEntryType::File;

        return fileData;
    }

    FileSystemEntry FileTree::fileSystemEntryFromDir(const std::filesystem::directory_entry& entry, std::string basePath)
    {
        if (!entry.is_directory()) {
            return {};
        }

        FileSystemEntry dirData{};
        dirData.name = entry.path().filename().string();
        dirData.relativePath = fs::relative(entry.path(), basePath).string();

        auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
        if (!absolutePathResult) {
            throw std::runtime_error(absolutePathResult.error().message());
        }

        dirData.absolutePath = absolutePathResult.value();

        // TODO: ignore node_modules and .git for now
        std::size_t found = dirData.absolutePath.find("node_modules");
        if (found != std::string::npos) {
            return {};
        }
        found = dirData.absolutePath.find(".git");
        if (found != std::string::npos) {
            return {};
        }
        //--

        fs::directory_iterator dirIter(entry.path());

        dirData.children = std::vector<FileSystemEntry>();

        size_t dirSize = 0;
        for (auto &entry : dirIter) {
            if (entry.is_symlink() || entry.is_other()) {
                continue;
            }

            if (entry.is_regular_file() ) {
                auto childFileData = fileSystemEntryFromFile(entry, basePath);
                dirSize += childFileData.size;
                dirData.children->push_back(childFileData);             
            }
            if (entry.is_directory()) {
                auto childDirData = fileSystemEntryFromDir(entry, basePath);
                dirSize += childDirData.size;
                dirData.children->push_back(childDirData);          
            }      
        }

        dirData.size = dirSize;
        dirData.type = FileSystemEntryType::Dir;

        return dirData;
    }

} // namespace backer