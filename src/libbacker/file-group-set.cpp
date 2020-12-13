#include "file-group-set.h"

#include "katla/core/posix-file.h"

#include <filesystem>
#include <openssl/md5.h>

namespace backer {

    namespace fs = std::filesystem;

    FileGroupSet::FileGroupSet() {
    }

    FileGroupSet FileGroupSet::create(std::string path) {
        FileGroupSet result;
        result.addAndGroupPotentialDuplicateFile(path);
        return result;
    }

    void FileGroupSet::addAndGroupPotentialDuplicateFile(std::string path) {
        fs::recursive_directory_iterator dirIter(path);
        for (auto &entry : dirIter) {
            if (entry.is_symlink() || entry.is_other()) {
                continue;
            }

            if (!entry.is_regular_file()) {
                continue;
            }

            FileSystemEntry fileData{};
            fileData.name = entry.path().filename().string();
            fileData.relativePath = fs::relative(entry.path(), path).string();

            auto absolutePathResult = katla::PosixFile::absolutePath(entry.path().string());
            if (!absolutePathResult) {
                throw std::runtime_error(absolutePathResult.error().message());
            }

            fileData.absolutePath = absolutePathResult.value();
            fileData.size = entry.file_size();

            std::string key = katla::format("{}-{}", fileData.name, fileData.size);

            auto findIt = m_fileMap.find(key);
            if (findIt != m_fileMap.end()) {
                findIt->second.push_back(fileData);
            } else {
                m_fileMap[key] = std::vector<FileSystemEntry>{fileData};
            }
        }
    }

    long FileGroupSet::countFiles()
    {
        long fileCount = 0;
        for (auto& pair : m_fileMap) {
            for(auto& file : pair.second) {
                fileCount++;
            }
        }

        return fileCount;
    }

} // namespace backer