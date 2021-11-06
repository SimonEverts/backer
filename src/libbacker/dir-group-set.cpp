#include "dir-group-set.h"

#include "core/core.h"
#include "file-data.h"
#include "katla/core/posix-file.h"

#include "backer.h"
#include "file-tree.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <openssl/md5.h>

namespace backer {

    DirGroupSet::DirGroupSet() = default;

    DirGroupSet DirGroupSet::createFromPath(std::string path, bool onlyTopDirs) {
        
        katla::printInfo("Indexing files");
        auto rootEntry = FileTree::create(path);

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

    std::vector<std::pair<std::weak_ptr<FileSystemEntry>, std::weak_ptr<FileSystemEntry>>>
    DirGroupSet::filterSubsetDirs(const std::vector<std::shared_ptr<backer::FileSystemEntry>>& flattenedList, bool onlyTopDirs)
    {
        // hashes duplicate files
        auto groupDuplicateFiles = listAndGroupDuplicateFiles(flattenedList);

        // for(auto& groupIt : groupDuplicateFiles) {
        //     if (groupIt.second.size() > 1) {
        //         katla::printInfo("Found {} duplicates:", groupIt.second.size());
        //         for(auto& dirIt : groupIt.second) {
        //             katla::printInfo("- {} {}", dirIt->absolutePath, groupIt.first);
        //         }
        //     }
        // }

        int total = groupDuplicateFiles.size();
        int c = 0;

        // get all parents from duplicate files as candidate dirs
        std::vector<std::pair<std::weak_ptr<FileSystemEntry>, std::weak_ptr<FileSystemEntry>>> resultList;
        for(const auto& groupIt : groupDuplicateFiles) {
            if (groupIt.second.size() <= 1) {
                continue;
            }

            katla::print(stdout, "Pressing duplicate {}/{} - {:.2f}% \r", c, total, float(c*100.0)/total);
            c++;

            std::map<std::string, std::weak_ptr<FileSystemEntry>> parents;
            for(const auto& dup : groupIt.second) {
                if (dup->type == FileSystemEntryType::Dir) {
                    continue;
                }

                if (!dup->parent.has_value()) {
                    continue;
                }
                auto parent = dup->parent.value();
                parents[parent.lock()->relativePath] = parent;
            }

            // for (auto& it : parents) {
            //     katla::printInfo("parent: {} {}", it.first, it.second.lock()->absolutePath);
            // }

            // TODO: include parents of parents?

            for(auto& parentItA : parents) {
                auto parentEntryA = parentItA.second.lock();
                if (parentEntryA->type == FileSystemEntryType::File) {
                    continue;
                }

                // TODO set minimum dir size for now to improve speed
                if (parentEntryA->size < 1024*1024*1024) {
                    continue;
                }

                // get list of all A's parents, so we can exclude them later
                std::vector<std::string> parentsOfA;
                std::optional<std::weak_ptr<FileSystemEntry>> parentIt = parentEntryA->parent;
                while(parentIt.has_value()) {
                    auto entry = parentIt.value().lock();
                    parentsOfA.push_back(entry->relativePath);
                    parentIt = entry->parent;
                }

                // katla::printInfo("parents of {}: {}", parentEntryA->relativePath, parentsOfA.size());
                // for(auto& it : parentsOfA) {
                //     katla::printInfo(" - {}", it);
                // }

                auto flatListA = FileTree::flatten(parentEntryA);

                // TODO set minimum dir size for now to improve speed
                if (flatListA.size() < 100) {
                    continue;
                }

                for(auto& parentItB : parents) {
                    if (parentItB.first == parentItA.first) {
                        continue;
                    }
                    
                    auto parentEntryB = parentItB.second.lock();

                    // skip if parentItB is parent of parentItA
                    auto findIt = std::find(parentsOfA.begin(), parentsOfA.end(), parentEntryB->relativePath);
                    if (findIt != parentsOfA.end()) {
                        continue;
                    }

                    auto flatListB = FileTree::flatten(parentEntryB);

                    std::map<std::string, std::shared_ptr<FileSystemEntry>> flatMapB;
                    for(auto& itListB : flatListB) {
                        flatMapB[itListB->relativePath] = itListB;
                    }

                    bool allAinB = true;
                    // check if pathB contains everything from pathA
                    for(auto& itA : flatListA) {
                        if (itA->type == FileSystemEntryType::Dir) {
                            continue;
                        }

                        auto trimmedPathA = katla::trimPrefix(itA->relativePath, parentEntryA->relativePath);
                        // katla::printInfo("trimA: {} {} {}", itA->relativePath, parentEntryA->relativePath, trimmedPathA);
                        bool itAinB = false;
                        for(auto& itB : flatMapB) {
                            if(itB.second->type == FileSystemEntryType::Dir) {
                                continue;
                            }

                            auto trimmedPathB = katla::trimPrefix(itB.second->relativePath, parentEntryB->relativePath);
                            // katla::printInfo("trimB: {} {} {}", itB.second->relativePath, parentEntryB->relativePath, trimmedPathB);

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
                        resultList.push_back({parentItA.second, parentItB.second});
                    }
                }
            }

        }

        //
        return resultList;
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