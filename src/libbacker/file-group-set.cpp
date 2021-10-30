#include "file-group-set.h"

#include "core/core.h"
#include "katla/core/posix-file.h"

#include "backer.h"
#include "file-tree.h"

#include <filesystem>
#include <memory>
#include <openssl/md5.h>

namespace backer {

    FileGroupSet::FileGroupSet() {
    }

    FileGroupSet FileGroupSet::createForFiles(std::string path) {
        FileGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateFiles(path);
        return result;
    }

    FileGroupSet FileGroupSet::createForDirs(std::string path, bool onlyTopDirs) {
        FileGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateDirs(path, onlyTopDirs);
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

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> FileGroupSet::listAndGroupDuplicateDirs(std::string path, bool onlyTopDirs)
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
        }

        if (onlyTopDirs) {
            removeDuplicateEntriesWithDuplicateParent(groupDirectories);
        }

        for(auto& groupIt : groupDirectories) {
            if (groupIt.second.size() > 1) {
                katla::printInfo("Found {} duplicates:", groupIt.second.size());
                for(auto& dirIt : groupIt.second) {
                    katla::printInfo("- {} {}", dirIt->absolutePath, groupIt.first);
                }
            }
        }

        return groupDirectories;
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

    void FileGroupSet::removeDuplicateEntriesWithDuplicateParent(std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& groupDirectories)
    {
        // for(auto& it : groupDirectories) {
        //     katla::printInfo("{}-{}", it.first, it.second.size());
        //     for(auto& x : it.second) {
        //         katla::printInfo(" - {}-{}", x->relativePath, backer::Backer::formatHash(x->hash));
        //     }
        // }

        // Remove subdir's with same parent hash
        // TODO own method
        for(auto& flatIt : groupDirectories) {
            auto& childList = flatIt.second;
            
            // Don't interested if current entry is not a duplicate
            if (childList.size() < 2) {
                continue;
            }

            std::vector<std::shared_ptr<FileSystemEntry>> removeList;
            for(auto& dubIt : childList) {
                if (!dubIt->parent.has_value()) {
                    continue;
                }

                auto parentHash = backer::Backer::formatHash(dubIt->parent->lock()->hash);
                auto key = katla::format("dir-{}", parentHash);

                // if parent is not in groupdirs and not a duplicate, skip to next
                auto findIt = groupDirectories.find(key);
                if (findIt == groupDirectories.end() || findIt->second.size() < 2) {
                    continue;
                }

                // katla::printInfo("remove child 1: {}", dubIt->absolutePath);
                removeList.push_back(dubIt);
            }

            for (auto& removeIt : removeList) {
                auto childIt = std::begin(childList);
                while (childIt != std::end(childList)) {
                    auto& parentOpt = (*childIt)->parent;
                    if (!parentOpt.has_value()) {
                        continue;
                    }
                    auto parent = parentOpt.value().lock();

                    auto childHash = backer::Backer::formatHash((*childIt)->hash);
                    auto parentHash = backer::Backer::formatHash(parent->hash);
                    
                    auto removeHash = backer::Backer::formatHash(removeIt->hash);
                    auto removeParentHash = backer::Backer::formatHash(removeIt->parent->lock()->hash);

                    // Remove childs with the same parent, but only for that parent
                    if (parentHash == removeParentHash && childHash == removeHash) {
                        // katla::printInfo("remove child 2: {}", (*childIt)->absolutePath);
                        childIt = childList.erase(childIt);
                    } else {
                        ++childIt;
                    }
                }                    
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