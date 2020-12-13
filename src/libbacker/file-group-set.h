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

    static FileGroupSet create(std::string path);

    void addAndGroupPotentialDuplicateFile(std::string path);

    long countFiles();

    std::map<std::string, std::vector<backer::FileSystemEntry>> fileMap() {
        return m_fileMap;
    }

private:
    std::map<std::string, std::vector<backer::FileSystemEntry>> m_fileMap;
};

} // namespace backer

#endif