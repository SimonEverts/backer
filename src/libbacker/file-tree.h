/***
 * Copyright 2019 The Katla Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FILE_TREE_H
#define FILE_TREE_H

#include "katla/core/core.h"

#include "file-data.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace backer {
    
class FileTree {
public:
    FileTree();

    static FileSystemEntry create(std::string path);

    static std::vector<backer::FileSystemEntry> flatten(const FileSystemEntry& entry);

    static backer::FileSystemEntry fileSystemEntryFromFile(const std::filesystem::directory_entry& entry, std::string basePath);
    static backer::FileSystemEntry fileSystemEntryFromDir(const std::filesystem::directory_entry& entry, std::string basePath);

private:
    static void flattenChild(const FileSystemEntry& entry, std::vector<backer::FileSystemEntry>& list);
};

} // namespace backer

#endif