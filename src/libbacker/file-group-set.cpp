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

    FileGroupSet FileGroupSet::create(std::string path) {
        FileGroupSet result;
        result.addAndGroupDuplicateFile(path);
        return result;
    }

    void FileGroupSet::addAndGroupDuplicateFile(std::string path) {

        katla::printInfo("Indexing files");
        auto rootEntry = FileTree::create(path);

        katla::printInfo("Flatten file list");
        auto flatList = FileTree::flatten(rootEntry);

        katla::printInfo("Group files based on name and size");
        std::map<std::string, std::vector<backer::FileSystemEntry>> nameSizeGroup;
        for (auto &entry : flatList) {

            // Dont process directories at this time
            if (entry.type == FileSystemEntryType::Dir) {
                continue;
            }

            std::string key = katla::format("{}-{}", entry.name, entry.size);

            auto findIt = nameSizeGroup.find(key);
            if (findIt != nameSizeGroup.end()) {
                findIt->second.push_back(entry);
            } else {
                nameSizeGroup[key] = std::vector<FileSystemEntry>{entry};
            }
        }

        katla::printInfo("Count files");
        auto nrOfFiles = countFiles(nameSizeGroup);

        katla::printInfo("Hash possible duplicate files");

        m_fileMap.clear();

        int fileNr = 0;
        for (auto &pair : nameSizeGroup) {
            auto &&fileGroup = pair.second;

            for (auto &file : fileGroup) {
                katla::print(stdout, "Hash possible duplicates: [{}/{}] {}\n", fileNr, nrOfFiles, file.absolutePath);
                fileNr++;

                if (file.name.empty()) {
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
                    m_fileMap[pair.first] = fileGroup;
                    continue;
                }

                file.hash = Backer::sha256(file.absolutePath);
                std::string newKey = katla::format("{}-{}", pair.first, backer::Backer::formatHash(file.hash));
                if (m_fileMap.find(newKey) == m_fileMap.end()) {
                    m_fileMap[newKey] = {};
                }
                m_fileMap[newKey].push_back(file);
            }
        }

        katla::printInfo("Hash and group directories");
        for (auto &entry : flatList) {

            // Dont process files at this time
            if (entry.type == FileSystemEntryType::File) {
                continue;
            }

            hashDir(entry);

            std::string key = katla::format("{}-{}", "dir", backer::Backer::formatHash(entry.hash));
            if (m_fileMap.find(key) == m_fileMap.end()) {
                m_fileMap[key] = {};
            }
            m_fileMap[key].push_back(entry);
        }
    }

    void FileGroupSet::hashDir(FileSystemEntry& currentEntry)
    {
        std::vector<std::vector<std::byte>> dirHashes;

        if (currentEntry.children.has_value()) {
            for(auto& childEntry : currentEntry.children.value()) {

                if (childEntry.type == FileSystemEntryType::Dir) {
                    hashDir(childEntry);
                }

                if (childEntry.hash.size()) {
                    dirHashes.push_back(childEntry.hash);
                } else {
                    // if file is unique based on name and size it doesn't have a hash defined. So use the key instead
                    std::string key = katla::format("{}-{}", childEntry.name, childEntry.size);
                    auto hash = Backer::sha256FromString(key);
                    dirHashes.push_back(hash);
                }
            }
        }

        currentEntry.hash = Backer::sha256(dirHashes);
    }

    long FileGroupSet::countFiles(const std::map<std::string, std::vector<backer::FileSystemEntry>>& fileMap) {
        long fileCount = 0;
        for (auto &pair : fileMap) {
            for (auto &file : pair.second) {
                fileCount++;
            }
        }

        return fileCount;
    }

} // namespace backer