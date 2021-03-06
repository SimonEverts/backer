#include "katla/core/core.h"
#include "katla/core/posix-file.h"

#include "libbacker/backer.h"
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

namespace fs = std::filesystem;



int main(int argc, char* argv[])
{
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

        fileGroupSet = backer::FileGroupSet::create(".");
        auto fileMap = fileGroupSet.fileMap();
        for (auto& pair : fileMap) {
            for(auto& file : pair.second) {
                katla::printInfo("{}", file.absolutePath);
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

        auto fileIndex = backer::FileIndexDatabase::create(fileIndexPath, path);

        return EXIT_SUCCESS;
    }

    
    if(argc == 3) {
//        katla::print(stdout, "Comparing src {} to dest {}\n", argv[1], argv[2]);
//
//        fileMap = backer::Backer::groupPotentialDuplicateFile(argv[1]);
//        backer::Backer::walkFiles(argv[2], fileMap, false, true);
    }

    backer::CountResult result {};
    result.nrOfFiles = fileGroupSet.countFiles();

    auto fileMap = fileGroupSet.fileMap();


    // Calculate md5 hashes of files in a group
    // - Use that md5 in the key to create an unique group
    std::map<std::string, std::vector<backer::FileSystemEntry>> uniqueGroup;
    int fileNr = 0;
    for (auto& pair : fileMap) {
        auto&& fileGroup = pair.second;

        for (auto& file : fileGroup) {
            katla::print(stdout, "[{}/{}] {}\n", fileNr, result.nrOfFiles, file.absolutePath);
            fileNr++;

            if (fileGroup.size() < 2) {
                uniqueGroup[pair.first] = fileGroup;
                continue;
            }

            file.hash = backer::Backer::sha256(file.absolutePath);
            std::string newKey = katla::format("{}-{}", pair.first, backer::Backer::formatHash(file.hash));
            if (uniqueGroup.find(newKey) == uniqueGroup.end()) {
                uniqueGroup[newKey] = {};
            }
            uniqueGroup[newKey].push_back(file);
        }
    }

    std::map<std::string, backer::FileSystemEntry> onlyAtSrc;


    for (auto& pair : uniqueGroup) {
        auto&& fileGroup = pair.second;

        bool noDest = true;
        for (auto& file : fileGroup) {
            if (file.isInDest) {
                noDest = false;
            }
        }

        if (noDest) {
            for (auto& file : fileGroup) {
                onlyAtSrc[file.absolutePath] = file;
            }
        } else {
            for (auto it = fileGroup.begin(); it != fileGroup.end(); it++) {
                result.atBoth++;
            }
        }
    }

    for (auto& pair : uniqueGroup) {
        auto fileGroup = pair.second;

        for(auto& file : fileGroup) {
            if (fileGroup.size() > 2) {
                result.duplicates++;
                katla::print(stdout, "Duplicates: {}-{}\n", backer::Backer::formatHash(file.hash), file.absolutePath);
            }
        }
    }

    backer::Backer::writeToFile("only-at-src.txt", onlyAtSrc);

    for (auto& pair : onlyAtSrc) {
        katla::print(stdout, "Only at src: {}\n", pair.first);
    }

    katla::print(stdout, "Nr of files: {}\n", result.nrOfFiles);
    katla::print(stdout, "Nr of files only at src: {}\n", onlyAtSrc.size());
    katla::print(stdout, "Nr of files at both: {}\n", result.atBoth);
    katla::print(stdout, "Nr of files have duplicates: {}\n", result.duplicates);
}
