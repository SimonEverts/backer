#include "file-group-set.h"

#include "katla/core/posix-file.h"

#include "backer.h"
#include "file-tree.h"

#include <filesystem>
#include <openssl/md5.h>

namespace backer {

    namespace fs = std::filesystem;

    FileGroupSet::FileGroupSet() {
    }

    FileGroupSet FileGroupSet::createForFiles(std::string path) {
        FileGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateFiles(path);
        return result;
    }

    FileGroupSet FileGroupSet::createForDirs(std::string path) {
        FileGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateDirs(path);
        return result;
    }

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> FileGroupSet::listAndGroupDuplicateFiles(std::string path) {

        katla::printInfo("Indexing files");
        auto rootEntry = std::shared_ptr<FileSystemEntry>(FileTree::create(path));

        katla::printInfo("Flatten file list");
        auto flatList = FileTree::flatten(rootEntry);

        katla::printInfo("Group files based on name and size");
        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> nameSizeGroup;
        for (auto &entry : flatList) {

            // Dont process directories at this time
            if (entry->type == FileSystemEntryType::Dir) {
                continue;
            }

            std::string key = katla::format("{}-{}", entry->name, entry->size);

            auto findIt = nameSizeGroup.find(key);
            if (findIt != nameSizeGroup.end()) {
                findIt->second.push_back(entry);
            } else {
                nameSizeGroup[key] = std::vector<std::shared_ptr<FileSystemEntry>>{entry};
            }
        }

        katla::printInfo("Count files");
        auto nrOfFiles = countFiles(nameSizeGroup);

        katla::printInfo("Hash possible duplicate files");

        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> groupDuplicateFiles;
        int fileNr = 0;
        for (auto &pair : nameSizeGroup) {
            auto &&fileGroup = pair.second;

            for (auto &file : fileGroup) {
                katla::print(stdout, "Hash possible duplicates: [{}/{}] {}\n", fileNr, nrOfFiles, file->absolutePath);
                fileNr++;

                if (file->name.empty()) {
                    continue;
                }

//                // TODO: ignore node_modules and .git for now
//                std::size_t found = file.absolutePath.find("node_modules");
//                if (found != std::string::npos) {
//                    continue;
//                }
//                found = file.absolutePath.find(".git");
//                if (found != std::string::npos) {
//                    continue;
//                }
//                //--

                if (fileGroup.size() < 2) {
                    groupDuplicateFiles[pair.first] = fileGroup;
                    continue;
                }

                file->hash = Backer::sha256(file->absolutePath);
                std::string newKey = katla::format("{}-{}", pair.first, backer::Backer::formatHash(file->hash));
                if (groupDuplicateFiles.find(newKey) == groupDuplicateFiles.end()) {
                    groupDuplicateFiles[newKey] = {};
                }
                groupDuplicateFiles[newKey].push_back(file);
            }
        }

        return groupDuplicateFiles;
    }

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> FileGroupSet::listAndGroupDuplicateDirs(std::string path)
    {
        katla::printInfo("Indexing files");
        auto rootEntry = std::shared_ptr<FileSystemEntry>(FileTree::create(path));

        katla::printInfo("Flatten file list");
        auto flatList = FileTree::flatten(rootEntry);

        katla::printInfo("Group files based on name and size");
        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> nameSizeGroup;
        for (auto &entry : flatList) {

            // Dont process directories at this time
            if (entry->type == FileSystemEntryType::Dir) {
                continue;
            }

            std::string key = katla::format("{}-{}", entry->name, entry->size);

            auto findIt = nameSizeGroup.find(key);
            if (findIt != nameSizeGroup.end()) {
                findIt->second.push_back(entry);
            } else {
                nameSizeGroup[key] = std::vector<std::shared_ptr<FileSystemEntry>>{entry};
            }
        }

        katla::printInfo("Count files");
        auto nrOfFiles = countFiles(nameSizeGroup);

        katla::printInfo("Hash possible duplicate files");

        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> groupDuplicateFiles;
        int fileNr = 0;
        for (auto &pair : nameSizeGroup) {
            auto &&fileGroup = pair.second;

            for (auto &file : fileGroup) {
                katla::print(stdout, "Hash possible duplicates: [{}/{}] {}\n", fileNr, nrOfFiles, file->absolutePath);
                fileNr++;

                if (file->name.empty()) {
                    continue;
                }

//                // TODO: ignore node_modules and .git for now
//                std::size_t found = file.absolutePath.find("node_modules");
//                if (found != std::string::npos) {
//                    continue;
//                }
//                found = file.absolutePath.find(".git");
//                if (found != std::string::npos) {
//                    continue;
//                }
//                //--

                if (fileGroup.size() < 2) {
                    groupDuplicateFiles[pair.first] = fileGroup;
                    continue;
                }

                file->hash = Backer::sha256(file->absolutePath);
                std::string newKey = katla::format("{}-{}", pair.first, backer::Backer::formatHash(file->hash));
                if (groupDuplicateFiles.find(newKey) == groupDuplicateFiles.end()) {
                    groupDuplicateFiles[newKey] = {};
                }
                groupDuplicateFiles[newKey].push_back(file);
            }
        }

        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> groupDirectories;

        katla::printInfo("Hash and group directories");
        for (auto &entry : flatList) {

            // Dont process files at this time
            if (entry->type == FileSystemEntryType::File) {
                continue;
            }

            hashDir(*entry);

            std::string key = katla::format("{}-{}", "dir", backer::Backer::formatHash(entry->hash));
            if (groupDirectories.find(key) == groupDirectories.end()) {
                groupDirectories[key] = {};
            }
            groupDirectories[key].push_back(entry);

            if (groupDirectories[key].size() > 1) {
                katla::printInfo("found duplicate dir: {} {} {}", groupDirectories[key].size(), entry->absolutePath, key);
            }
        }


        return groupDirectories;

        // TODO

        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> toplevelDirectories;
        findTopLevelDuplicateDirs(rootEntry, groupDirectories, toplevelDirectories);
        m_fileMap = toplevelDirectories;

        katla::printInfo("found duplicate dirs: {}", toplevelDirectories.size());

        return toplevelDirectories;
    }

    void FileGroupSet::hashDir(FileSystemEntry& currentEntry)
    {
        std::vector<std::vector<std::byte>> dirHashes;

        if (currentEntry.children.has_value()) {
            for(auto& childEntry : currentEntry.children.value()) {

                if (childEntry->type == FileSystemEntryType::Dir) {
                    hashDir(*childEntry);
                }

                if (childEntry->hash.size()) {
                    dirHashes.push_back(childEntry->hash);
                } else {
                    // TODO only in single collection!
                    // if file is unique based on name and size it doesn't have a hash defined. So use the key instead
                    std::string key = katla::format("{}-{}", childEntry->name, childEntry->size);
                    auto hash = Backer::sha256FromString(key);
                    dirHashes.push_back(hash);
                }
            }
        }

        currentEntry.hash = Backer::sha256(dirHashes);
    }

    void FileGroupSet::findTopLevelDuplicateDirs(
            std::shared_ptr<FileSystemEntry>& entry,
            std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& flatList,
            std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& topLevelDirectories
            )
    {
        if (entry->type == FileSystemEntryType::File) {
            return;
        }

        auto hashFromEntry = [](const FileSystemEntry& entry) {
            if (entry.type == FileSystemEntryType::Dir) {
                return katla::format("{}-{}", "dir", backer::Backer::formatHash(entry.hash));
            }

            auto key = katla::format("{}-{}", entry.name, entry.size);
            if (entry.hash.empty()) {
                return key;
            }

            return katla::format("{}-{}", key, backer::Backer::formatHash(entry.hash));
        };


        auto key = hashFromEntry(*entry);

        katla::printInfo("dir key: {}", key);

        {
            auto it = flatList.find(key);
            if (it != flatList.end() && it->second.size() > 1) {
                topLevelDirectories[key] = it->second;
                katla::printInfo("found toplevel dir: {}, {}", key, it->second.size());
                return;
            }
        }

        if (entry->children.has_value()) {
            for(auto& childEntry : entry->children.value()) {

                if (childEntry->type != FileSystemEntryType::Dir) {
                    continue;
                }

                findTopLevelDuplicateDirs(childEntry, flatList, topLevelDirectories);
            }
        }

    }

    long FileGroupSet::countFiles(const std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& fileMap) {
        long fileCount = 0;
        for (auto &pair : fileMap) {
            for (auto &file : pair.second) {
                fileCount++;
            }
        }

        return fileCount;
    }

} // namespace backer