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

            // order of children is important, must be sorted
            std::map<std::string, std::shared_ptr<FileSystemEntry>> sortedChildren;
            for(auto& childEntry : currentEntry.children.value()) {
                sortedChildren[childEntry->relativePath] = childEntry;
            }

            for(auto& childEntry : sortedChildren) {

                if (childEntry.second->type == FileSystemEntryType::Dir) {
                    hashDir(*childEntry.second);
                }

                if (childEntry.second->hash.has_value() && childEntry.second->hash.value().size()) {
                    dirHashes.push_back(childEntry.second->hash.value());
                } else {
                    // TODO only in single collection!
                    // if file is unique based on name and size it doesn't have a hash defined. So use the key instead
                    std::string key = katla::format("{}-{}", childEntry.second->name, childEntry.second->size);
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