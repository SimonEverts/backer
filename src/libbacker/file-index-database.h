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

#ifndef FILE_INDEX_DATABASE_H
#define FILE_INDEX_DATABASE_H

#include "katla/core/core.h"
#include "katla/sqlite/sqlite-database.h"

#include "file-data.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace backer {

class FileIndexDatabase {
public:
    FileIndexDatabase();

    static FileIndexDatabase create(std::string indexDatabasePath, std::string indexSource);

private:
    void createSqliteDatabase(std::string path);
    void fillDatabase(std::string path);

    std::vector<std::byte> processEntry(const FileSystemEntry& entry, std::vector<std::pair<std::string, std::vector<std::byte>>>& fileHashList, size_t& idx, size_t totalCount);

    katla::SqliteDatabase m_database;
};

} // namespace backer

#endif