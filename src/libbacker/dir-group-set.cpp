#include "dir-group-set.h"

#include "core/core.h"
#include "katla/core/posix-file.h"

#include "backer.h"
#include "file-tree.h"

#include <filesystem>
#include <memory>
#include <openssl/md5.h>

namespace backer {

    DirGroupSet::DirGroupSet() = default;

    DirGroupSet DirGroupSet::createFromPath(std::string path, bool onlyTopDirs) {
        
        katla::printInfo("Indexing files");
        auto rootEntry = std::shared_ptr<FileSystemEntry>(FileTree::create(path));

        katla::printInfo("Flatten file list");
        auto flatList = FileTree::flatten(rootEntry);
        
        DirGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateDirs(flatList, onlyTopDirs);
        return result;
    }

    DirGroupSet DirGroupSet::createFromFlattenedList(const std::vector<std::shared_ptr<backer::FileSystemEntry>>& flattenedList, bool onlyTopDirs) {
        DirGroupSet result;
        result.m_fileMap = result.listAndGroupDuplicateDirs(flattenedList, onlyTopDirs);
        return result;
    }

    std::vector<std::pair<std::shared_ptr<FileSystemEntry>, std::shared_ptr<FileSystemEntry>>>
    DirGroupSet::filterSubsetDirs(const std::vector<std::shared_ptr<backer::FileSystemEntry>>& flattenedList, bool onlyTopDirs)
    {
        // hashes duplicate files
        auto groupDuplicateFiles = listAndGroupDuplicateFiles(flattenedList);


        // get all parents from duplicate files as candidate dirs
        for(auto dp)

        //
        std::vector<std::pair<std::shared_ptr<FileSystemEntry>, std::shared_ptr<FileSystemEntry>>> resultList;
        for(auto& it : flattenedList) {
            if (it->type == FileSystemEntryType::File) {
                continue;
            }

            auto groupIt = groupDuplicateFiles.find(backer::Backer::formatHash(it->hash.value()));
            if (groupIt == groupDuplicateFiles.end()) {
                continue;
            }


            auto flatListA = FileTree::flatten(it);
            auto relPathA = it->parent.value().lock()->relativePath;

            for(auto& dupIt : groupIt->second) {
                auto flatListB = FileTree::flatten(dupIt);

                std::map<std::string, std::shared_ptr<FileSystemEntry>> flatMapB;
                for(auto& itListB : flatListB) {
                    flatMapB[itListB->relativePath] = itListB;
                }

                auto relPathB = dupIt->parent->lock()->relativePath;

                bool allAinB = true;
                // check if pathB contains everything from pathA
                for(auto& itA : flatListA) {
                    auto trimmedPathA = katla::trimPrefix(itA->relativePath, relPathA);
                    katla::printInfo("trimA: {} {} {}", itA->relativePath, relPathA, trimmedPathA);
                    bool itAinB = false;
                    for(auto& itB : flatMapB) {
                        auto trimmedPathB = katla::trimPrefix(itB.second->relativePath, relPathB);
                        katla::printInfo("trimB: {} {} {}", itB.second->relativePath, relPathB, trimmedPathB);

                        if (trimmedPathA == trimmedPathB &&
                                backer::Backer::formatHash(itA->hash.value()) == backer::Backer::formatHash(itB.second->hash.value())) {
                            itAinB = true;
                        }
                    }

                    if (!itAinB) {
                        allAinB = false;
                    }
                }

                if(allAinB) {
                    resultList.push_back({it, dupIt});
                }
            }
        }
    }

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>
    DirGroupSet::listAndGroupDuplicateDirs(const std::vector<std::shared_ptr<backer::FileSystemEntry>>& flattenedList, bool onlyTopDirs)
    {
        // hashes duplicate files
        listAndGroupDuplicateFiles(flattenedList);
        
        std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> groupDirectories;

        katla::printInfo("Hash and group directories");
        for (auto &entry : flattenedList) {

            // Dont process files at this time
            if (entry->type == FileSystemEntryType::File) {
                continue;
            }

            hashDir(*entry);

            std::string key = katla::format("{}-{}", "dir", backer::Backer::formatHash(entry->hash.value()));
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

    void DirGroupSet::removeDuplicateEntriesWithDuplicateParent(std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& groupDirectories)
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

                auto parentHash = backer::Backer::formatHash(dubIt->parent->lock()->hash.value());
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

                    auto childHash = backer::Backer::formatHash((*childIt)->hash.value());
                    auto parentHash = backer::Backer::formatHash(parent->hash.value());
                    
                    auto removeHash = backer::Backer::formatHash(removeIt->hash.value());
                    auto removeParentHash = backer::Backer::formatHash(removeIt->parent->lock()->hash.value());

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

} // namespace backer