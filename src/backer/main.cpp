#include "katla/core/core.h"
#include "katla/core/posix-file.h"

#include "libbacker/backer.h"
#include "libbacker/file-tree.h"
#include "libbacker/file-group-set.h"
#include "libbacker/file-index-database.h"

#include "cxxopts.hpp"

#include <openssl/md5.h>

#include <vector>
#include <string>
#include <map>
#include <filesystem>

#include <cstdio>
#include <cstdint>
#include <cstddef>

#include <exception>

int main(int argc, char* argv[])
{
    try {
        cxxopts::Options options("Backer", "Backup toolkit");

        options.add_options()
                ("h,help", "Print help")
                ("s,source", "Source path", cxxopts::value<std::string>())
                ("c,command", "Specify command, options are: {list}", cxxopts::value<std::string>())
                ("o,output", "Output file", cxxopts::value<std::string>())
                ("a,args", "last tmp", cxxopts::value<std::vector<std::string>>());
        options.parse_positional({"command", "args"});

        auto optionsResult = options.parse(argc, argv);

        if (optionsResult.count("help")) {
            katla::print(stdout, options.help());
            return EXIT_SUCCESS;
        }

        auto command = optionsResult["command"].as<std::string>();

        backer::FileGroupSet fileGroupSet;

        if (command == "list") {
            std::string path = ".";
            if (optionsResult.count("args")) {
                auto arguments = optionsResult["args"].as<std::vector<std::string>>();
                if (arguments.size()) {
                    path = arguments.front();
                }
            }

            fileGroupSet = backer::FileGroupSet::createFromPath(".");
            auto fileMap = fileGroupSet.fileMap();
            for (auto &pair : fileMap) {
                for (auto &file : pair.second) {
                    katla::printInfo("{}", file->absolutePath);
                }
            }

            return EXIT_SUCCESS;
        }

        if (command == "create-file-index") {
            std::string path = ".";
            if (optionsResult.count("args")) {
                auto arguments = optionsResult["args"].as<std::vector<std::string>>();
                if (arguments.size()) {
                    path = arguments.front();
                }
            }
            if (optionsResult.count("source")) {
                path = optionsResult["source"].as<std::string>();
            }

            auto fileIndexPath = katla::format("{}/file-index.db.sqlite", path);
            if (optionsResult.count("output")) {
                fileIndexPath = optionsResult["output"].as<std::string>();
            }

            if (std::filesystem::exists(fileIndexPath)) {
                std::filesystem::remove(fileIndexPath);
            }

            auto fileIndex = backer::FileIndexDatabase::create(fileIndexPath, path);

            return EXIT_SUCCESS;
        }

        if (command == "list-file-index") {
            try {
                std::string path = ".";
                if (optionsResult.count("args")) {
                    auto arguments = optionsResult["args"].as<std::vector<std::string>>();
                    if (arguments.size()) {
                        path = arguments.front();
                    }
                }
                if (optionsResult.count("source")) {
                    path = optionsResult["source"].as<std::string>();
                }

                auto fileIndexPath = katla::format("{}/file-index.db.sqlite", path);

                auto fileIndex = backer::FileIndexDatabase::open(fileIndexPath);
                auto fileList = fileIndex.getFileIndex();

                for (auto &it : fileList) {
                    katla::printInfo("{:<20} {}", backer::Backer::formatHash(it.hash.value()), it.relativePath);
                }

            } catch (std::runtime_error ex) {
                katla::printError(ex.what());
            }
        }

        if (command == "compare-file-index") {
            try {
                std::string path1 = ".";
                std::string path2 = ".";
                if (optionsResult.count("args") >= 2) {
                    auto arguments = optionsResult["args"].as<std::vector<std::string>>();
                    if (arguments.size()) {
                        path1 = arguments[0];
                        path2 = arguments[1];
                    }
                }

                auto fileIndexPath1 = katla::format("{}/file-index.db.sqlite", path1);
                auto fileIndexPath2 = katla::format("{}/file-index.db.sqlite", path2);

                auto fileIndex1 = backer::FileIndexDatabase::open(fileIndexPath1);
                auto fileList1 = fileIndex1.getFileIndex();

                // create maps based on relative path
                std::map<std::string, backer::FileSystemEntry> fileMap1;
                for(auto& it : fileList1) {
                    fileMap1[it.relativePath] = it;
                }

                auto fileIndex2 = backer::FileIndexDatabase::open(fileIndexPath2);
                auto fileList2 = fileIndex2.getFileIndex();

                std::map<std::string, backer::FileSystemEntry> fileMap2;
                for(auto& it : fileList2) {
                    fileMap2[it.relativePath] = it;
                }

                // compare files using relative-path
                std::vector<backer::FileSystemEntry> onlyIn1;
                std::vector<std::pair<backer::FileSystemEntry, backer::FileSystemEntry>> diff;

                for (auto &it : fileMap1) {
                    auto findIt = fileMap2.find(it.first);
                    if (findIt == fileMap2.end()) {
                        onlyIn1.push_back(it.second);
                        continue;
                    }

                    if (backer::Backer::formatHash(it.second.hash.value()) != backer::Backer::formatHash(findIt->second.hash.value()) ) {

                        // TODO tmp
                        if(it.second.type != backer::FileSystemEntryType::Dir) {
                            diff.push_back({it.second, findIt->second});
                            katla::printInfo("- {} {} {} {} {} {}", it.second.relativePath, findIt->second.relativePath, it.second.size, findIt->second.size, backer::Backer::formatHash(it.second.hash.value()), backer::Backer::formatHash(findIt->second.hash.value()));
                        }

                        // for(auto& fileIt : )
                    }
                }

                std::vector<backer::FileSystemEntry> onlyIn2;
                for (auto &it : fileMap2) {
                    auto findIt = fileMap1.find(it.first);
                    if (findIt == fileMap1.end()) {
                        onlyIn2.push_back(it.second);
                        continue;
                    }
                }

                katla::printInfo("Different:");
                for (auto& it : diff) {
                    katla::printInfo("  {}", it.first.relativePath);
                }

                katla::printInfo("\nOnly in 1:");
                for (auto& it : onlyIn1) {
                    katla::printInfo("  {}", it.relativePath);
                }

                katla::printInfo("\nOnly in 2:");
                for (auto& it : onlyIn2) {
                    katla::printInfo("  {}", it.relativePath);
                }

            } catch (std::runtime_error ex) {
                katla::printError(ex.what());
            }
        }


        if (command == "find-duplicate-dirs-from-index") {
            try {
                std::string path = ".";
                if (optionsResult.count("args")) {
                    auto arguments = optionsResult["args"].as<std::vector<std::string>>();
                    if (arguments.size()) {
                        path = arguments.front();
                    }
                }
                if (optionsResult.count("source")) {
                    path = optionsResult["source"].as<std::string>();
                }

                auto fileIndexPath = katla::format("{}/file-index.db.sqlite", path);

                auto fileIndex = backer::FileIndexDatabase::open(fileIndexPath);
                auto fileList = fileIndex.getFileIndex();

                auto fileTree = backer::FileTree::create(path);
                backer::FileTree::fillDataFromIndex(*fileTree, fileList);
                backer::FileTree::recursiveHash(*fileTree);

                auto flatList = backer::FileTree::flatten(fileTree);

                auto dirGroup = backer::DirGroupSet::createFromFlattenedList(flatList, true);
                auto dupList = dirGroup.fileMap();

                std::map<long long, std::vector<std::vector<std::shared_ptr<backer::FileSystemEntry>>>> sortedDupGroup;

                for(auto& groupIt : dupList) {
                    if(groupIt.second.size() < 2) {
                        continue;
                    }
                    auto size = groupIt.second.front()->size;
                    
                    auto findIt = sortedDupGroup.find(size);
                    if (findIt == sortedDupGroup.end()) {
                        sortedDupGroup[size] = {};
                    }

                    sortedDupGroup[size].push_back(groupIt.second);
                }

                katla::printInfo("Duplicates:");
                for(auto& sizeIt : sortedDupGroup) {
                    for(auto& groupIt : sizeIt.second) {
                        katla::printInfo("-");
                        for(auto& entryIt : groupIt) {
                            katla::printInfo("  {:<20} {} {:<20}", katla::humanFileSize(entryIt->size), entryIt->relativePath, entryIt->hash.has_value() ? backer::Backer::formatHash(entryIt->hash.value()) : "");
                        }
                    }
                }

                // std::function<void(backer::FileSystemEntry&)> printRecursiveTree;
                // printRecursiveTree = [&printRecursiveTree](backer::FileSystemEntry& entry) {
                //     katla::printInfo("{:<20} {}", entry.hash.has_value() ? backer::Backer::formatHash(entry.hash.value()) : "", entry.relativePath);
                //     if(entry.type == backer::FileSystemEntryType::Dir) {
                //         for(auto& it : entry.children.value()) {
                //             printRecursiveTree(*it);
                //         }
                //     }
                // };

                // printRecursiveTree(*fileTree);

            } catch (std::runtime_error ex) {
                katla::printError(ex.what());
            }
        }

        // fileGroupSet = backer::FileGroupSet::create("."); // /mnt/exthdd/primary
        // fileMap = fileGroupSet.fileMap();

        // for (auto& pair : fileMap) {
        //     if (pair.second.size() < 2) {
        //         continue;
        //     }

        //     auto front = pair.second.front();
        //     if (front->type != backer::FileSystemEntryType::Dir) {
        //         continue;
        //     }

        //     keys.push_back(pair.first);
        //         listview->addWidget(genListEntry(*pair.second.front()));
        // }
        // }

        //     listview->onRowSelected([&fileMap, &keys, &duplicateListView, &genListEntry](int index) {
        //         katla::printInfo("Row selected {}", index);

        //         duplicateListView->clear();
        //         for (auto& file : fileMap[keys[index]]) {
        //             duplicateListView->addWidget(genListEntry(*file));
        //         }
        //     });


        backer::CountResult result{};
        //    result.nrOfFiles = fileGroupSet.countFiles();

        //    auto fileMap = fileGroupSet.fileMap();
        //
        //
        //    // Calculate md5 hashes of files in a group
        //    // - Use that md5 in the key to create an unique group
        //    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> uniqueGroup;
        //    int fileNr = 0;
        //    for (auto& pair : fileMap) {
        //        auto&& fileGroup = pair.second;
        //
        //        for (auto& file : fileGroup) {
        //            katla::print(stdout, "[{}/{}] {}\n", fileNr, result.nrOfFiles, file->absolutePath);
        //            fileNr++;
        //
        //            if (fileGroup.size() < 2) {
        //                uniqueGroup[pair.first] = fileGroup;
        //                continue;
        //            }
        //
        //            file->hash = backer::Backer::sha256(file->absolutePath);
        //            std::string newKey = katla::format("{}-{}", pair.first, backer::Backer::formatHash(file->hash));
        //            if (uniqueGroup.find(newKey) == uniqueGroup.end()) {
        //                uniqueGroup[newKey] = {};
        //            }
        //            uniqueGroup[newKey].push_back(file);
        //        }
        //    }
        //
        //    std::map<std::string, backer::FileSystemEntry> onlyAtSrc;
        //
        //
        //    for (auto& pair : uniqueGroup) {
        //        auto&& fileGroup = pair.second;
        //
        //        bool noDest = true;
        //        for (auto& file : fileGroup) {
        //            if (file->isInDest) {
        //                noDest = false;
        //            }
        //        }
        //
        //        if (noDest) {
        //            for (auto& file : fileGroup) {
        //                onlyAtSrc[file.absolutePath] = file;
        //            }
        //        } else {
        //            for (auto it = fileGroup.begin(); it != fileGroup.end(); it++) {
        //                result.atBoth++;
        //            }
        //        }
        //    }
        //
        //    for (auto& pair : uniqueGroup) {
        //        auto fileGroup = pair.second;
        //
        //        for(auto& file : fileGroup) {
        //            if (fileGroup.size() > 2) {
        //                result.duplicates++;
        //                katla::print(stdout, "Duplicates: {}-{}\n", backer::Backer::formatHash(file.hash), file.absolutePath);
        //            }
        //        }
        //    }
        //
        //    backer::Backer::writeToFile("only-at-src.txt", onlyAtSrc);
        //
        //    for (auto& pair : onlyAtSrc) {
        //        katla::print(stdout, "Only at src: {}\n", pair.first);
        //    }
        //
        //    katla::print(stdout, "Nr of files: {}\n", result.nrOfFiles);
        //    katla::print(stdout, "Nr of files only at src: {}\n", onlyAtSrc.size());
        //    katla::print(stdout, "Nr of files at both: {}\n", result.atBoth);
        //    katla::print(stdout, "Nr of files have duplicates: {}\n", result.duplicates);
    } catch (const std::runtime_error& ex) {
        katla::printError("Exception caught: {}", ex.what());
    }
}
