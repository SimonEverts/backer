#include "base-group-set.h"

#include "core/core.h"
#include "katla/core/posix-file.h"

#include "backer.h"
#include "file-tree.h"

#include <filesystem>
#include <memory>
#include <openssl/md5.h>

namespace backer {

    BaseGroupSet::BaseGroupSet() = default;

    void BaseGroupSet::hashDir(FileSystemEntry& currentEntry)
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

    long BaseGroupSet::count(const std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& fileMap) {
        long fileCount = 0;
        for (auto &pair : fileMap) {
            for (auto &file : pair.second) {
                fileCount++;
            }
        }

        return fileCount;
    }

} // namespace backer