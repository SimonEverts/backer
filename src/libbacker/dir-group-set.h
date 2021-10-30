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

#ifndef DIR_GROUP_SET_H
#define DIR_GROUP_SET_H

#include "katla/core/core.h"

#include "base-group-set.h"
#include "file-data.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace backer {

class DirGroupSet : public BaseGroupSet {
public:
    DirGroupSet();

    static DirGroupSet createFromPath(std::string path, bool onlyTopDirs);
    std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>> listAndGroupDuplicateDirs(std::string path, bool onlyTopDirs);

private:
    void removeDuplicateEntriesWithDuplicateParent(std::map<std::string, std::vector<std::shared_ptr<backer::FileSystemEntry>>>& groupedDirectories);
};

} // namespace backer

#endif