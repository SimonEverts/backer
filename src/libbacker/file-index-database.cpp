#include "file-index-database.h"

#include "file-group-set.h"
#include "file-tree.h"

#include "katla/core/posix-file.h"
#include "backer.h"

#include <filesystem>
#include <openssl/md5.h>

#include <exception>

namespace backer {

    namespace fs = std::filesystem;

    FileIndexDatabase::FileIndexDatabase() {
    }

    FileIndexDatabase FileIndexDatabase::create(std::string indexDatabasePath, std::string indexSource) {
        FileIndexDatabase result;

        result.createSqliteDatabase(indexDatabasePath);
        result.fillDatabase(indexSource);
        return result;
    }

    void FileIndexDatabase::createSqliteDatabase(std::string path) {
        katla::printInfo("Creating file index: {}", path);

        auto createResult = m_database.create(path);
        if (!createResult) {
            throw std::runtime_error(katla::format("Failed creating file-index: {}", createResult.error().message()));
        }

        m_database.init();
    }

    void FileIndexDatabase::fillDatabase(std::string path) {
        katla::printInfo("Retreiving file list...");

        auto fileSystemEntry = std::shared_ptr<FileSystemEntry>(FileTree::create(path));

        auto flatList = FileTree::flatten(fileSystemEntry);

        std::vector<std::pair<std::string, std::vector<std::byte>>> fileHashList;

        size_t idx = 0;
        processEntry(*fileSystemEntry, fileHashList, idx, flatList.size());
        
        auto openResult = m_database.open();
        if (!openResult) {
            katla::printError("Opering table failed!: {}", openResult.error().message());
            return;
        }

        std::string sqlCreateTableQuery = katla::format("CREATE TABLE IF NOT EXISTS fileIndex (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, file TEXT, sha256 TEXT);");
        auto createTableQueryResult = m_database.exec(sqlCreateTableQuery);
        if (!createTableQueryResult) {
            katla::printError("Creating table failed!: {}", createTableQueryResult.error().message());
            return;
        }

        constexpr int BatchSize = 1024;
        const int fileHashListSize = fileHashList.size();

        for (int i = 0; i < fileHashList.size(); i += BatchSize) {

            auto beginTransactionResult = m_database.exec("BEGIN TRANSACTION;");
            if (!beginTransactionResult) {
                katla::printError("Error beginning transaction!: {}", beginTransactionResult.error().message());
                return;
            }

            for (int batchIndex = 0; batchIndex < BatchSize; batchIndex++) {
                if ((i + batchIndex) >= fileHashList.size()) {
                    break;
                }

                auto& pair = fileHashList[i + batchIndex];
                if (pair.first.size() == 0) {
                    continue;
                }

                std::vector<std::pair<std::string, std::string>> values = {{"file", pair.first}, {"sha256", Backer::formatHash(pair.second)}};

                auto insertQueryResult = m_database.insert("fileIndex", values);
                if (!insertQueryResult) {
                    katla::printError("Inserting into fileindex failed!: {} - {}", insertQueryResult.error().message(), insertQueryResult.error().description());
                    return;
                }
            }

            katla::printInfo("Saving database: {}/{}", i+1, fileHashListSize);

            auto commitTransactionResult = m_database.exec("COMMIT TRANSACTION;");
            if (!commitTransactionResult) {
                katla::printError("Error commiting transaction!: {} - {}", commitTransactionResult.error().message(), commitTransactionResult.error().description());
                return;
            }
        }

        m_database.close();
    }

    std::vector<std::byte> FileIndexDatabase::processEntry(const FileSystemEntry& entry, std::vector<std::pair<std::string, std::vector<std::byte>>>& fileHashList, size_t& idx, size_t totalCount)
    {
        if (entry.type == FileSystemEntryType::File) {
            auto sha256 = Backer::sha256(entry.absolutePath);

            fileHashList.push_back({entry.relativePath, sha256});
            katla::printInfo("{}/{} {}", ++idx, totalCount, entry.relativePath);
            return sha256;
        }

        if (entry.type != FileSystemEntryType::Dir) {
            return {};
        }

        std::vector<std::vector<std::byte>> dirHashes;
        for (auto& file : entry.children.value()) {
            auto processResult = processEntry(*file, fileHashList, idx, totalCount);
            dirHashes.push_back(processResult);
        }

        katla::printInfo("{}/{} {}", ++idx, totalCount, entry.relativePath);
        auto result = Backer::sha256(dirHashes);
        fileHashList.push_back({entry.relativePath, result});
        return result;
    }

} // namespace backer