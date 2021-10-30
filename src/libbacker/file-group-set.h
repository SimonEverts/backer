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

#ifndef FILE_GROUP_SET_H
#define FILE_GROUP_SET_H

#include "katla/core/core.h"

#include "file-data.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace backer {

class FileGroupSet {
public:
    FileGroupSet();

    static FileGroupSet createForFiles(std::string path);
    static FileGroupSet createForDirs(std::string path, bool onlyTopDirs);

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> listAndGroupDuplicateFiles(std::string path);
    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> listAndGroupDuplicateDirs(std::string path, bool onlyTopDirs);

    long countFiles(const std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& fileMap);

    void hashDir(FileSystemEntry& entry);

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> fileMap() {
        return m_fileMap;
    }

private:
    void removeDuplicateEntriesWithDuplicateParent(std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& groupedDirectories);

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> m_fileMap;
};

} // namespace backer

#endif