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

#ifndef BACKER_H
#define BACKER_H

#include "katla/core/core.h"

#include "file-group-set.h"
#include "dir-group-set.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace backer {

struct CountResult
{
    int nrOfFiles { 0 };
    int onlyAtSrc { 0 };
    int atBoth { 0 };
    int duplicates { 0 };
};

class Backer {
  public:
    static std::vector<std::byte> sha256(std::string path);
    static std::vector<std::byte> sha256(std::vector<std::vector<std::byte>> hashes);
    static std::vector<std::byte> sha256FromString(std::string str);

    static std::string formatHash(const std::vector<std::byte>& hash);
    static std::vector<std::byte> parseHashString(const std::string& hashString);

    static void writeToFile(std::string filePath, std::map<std::string, FileSystemEntry>& fileData);
};

} // namespace backer

#endif