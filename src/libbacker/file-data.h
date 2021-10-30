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

#ifndef FILE_DATA_H
#define FILE_DATA_H

#include "katla/core/core.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace backer {

enum class FileSystemEntryType { File, Dir };

struct FileSystemEntry
{
    std::string name;
    std::string relativePath;
    std::string absolutePath;
    uint64_t size { 0 };
    std::optional<std::vector<std::byte>> hash;

    FileSystemEntryType type {FileSystemEntryType::File};

    std::optional<std::weak_ptr<FileSystemEntry>> parent;
    std::optional<std::vector<std::shared_ptr<FileSystemEntry>>> children;
};

} // namespace backer

#endif