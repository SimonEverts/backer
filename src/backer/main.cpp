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

                katla::printInfo("Duplicates:");
                for(auto& groupIt : dupList) {
                    if(groupIt.second.size() < 2) {
                        continue;
                    }
                    katla::printInfo("-");
                    for(auto& entryIt : groupIt.second) {
                        katla::printInfo("  {:<20} {}", entryIt->hash.has_value() ? backer::Backer::formatHash(entryIt->hash.value()) : "", entryIt->relativePath);
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


        if (argc == 3) {
            //        katla::print(stdout, "Comparing src {} to dest {}\n", argv[1], argv[2]);
            //
            //        fileMap = backer::Backer::groupPotentialDuplicateFile(argv[1]);
            //        backer::Backer::walkFiles(argv[2], fileMap, false, true);
        }

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
