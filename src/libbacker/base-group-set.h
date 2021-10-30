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

#ifndef BASE_GROUP_SET_H
#define BASE_GROUP_SET_H

#include "file-data.h"
#include "katla/core/core.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace backer {

class BaseGroupSet {
  public:
    BaseGroupSet();

    long count(const std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& fileMap);
    void hashDir(FileSystemEntry& entry);

    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> fileMap() { return m_fileMap; }

  protected:
    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> m_fileMap;
};

} // namespace backer

#endif