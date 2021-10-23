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

    FileIndexDatabase FileIndexDatabase::open(std::string path) {
        FileIndexDatabase result;

        result.openSqliteDatabase(path);
        return result;
    }

    void FileIndexDatabase::createSqliteDatabase(std::string path) {
        katla::printInfo("Creating file index: {}", path);

        // m_database.init();
        auto createResult = m_database.create(path);
        if (!createResult) {
            throw std::runtime_error(katla::format("Failed creating file-index: {}", createResult.error().message()));
        }
    }

    void FileIndexDatabase::openSqliteDatabase(std::string path) {
        katla::printInfo("Opening file index: {}", path);

        // m_database.init();
        auto openResult = m_database.open(path);
        if (!openResult) {
            throw std::runtime_error(katla::format("Failed opening file-index: {}", openResult.error().message()));
        }
    }

    std::vector<std::pair<std::string, std::vector<std::byte>>> FileIndexDatabase::getFileIndex() {

        auto queryResult = m_database.exec( "SELECT * FROM fileIndex;");
        if (queryResult.has_error()) {
            katla::printError("{}: {}", queryResult.error().message(), queryResult.error().description());
            return {};
        }

        auto columnNames = queryResult.value().queryResult->columnNames;

        auto& data = queryResult.value().queryResult->data;
        auto nrOfColumns = queryResult.value().queryResult->nrOfColumns;

        std::vector<std::pair<std::string, std::vector<std::byte>>> result;
        for(int r=0; r<data.size(); r+= nrOfColumns) {
            std::string filePath;
            std::vector<std::byte> sha256;

            for(int c=0; c<nrOfColumns; c++) {
                if (columnNames[c] == "file") {
                    filePath = data[r+c];
                }

                if (columnNames[c] == "sha256") {
                    sha256 = Backer::parseHashString(data[r+c]);
                }
            }

            result.push_back({filePath, sha256});
        }

        m_database.close();

        return result;
    }

    void FileIndexDatabase::fillDatabase(std::string path) {
        katla::printInfo("Retreiving file list...");

        auto fileSystemEntry = std::shared_ptr<FileSystemEntry>(FileTree::create(path));

        auto flatList = FileTree::flatten(fileSystemEntry);

        FileTree::recursiveHash(*fileSystemEntry);
        
        auto openResult = m_database.open();
        if (!openResult) {
            katla::printError("Opering table failed!: {}", openResult.error().message());
            return;
        }

        std::string sqlCreateTableQuery = katla::format("CREATE TABLE IF NOT EXISTS fileIndex (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, relativePath TEXT, isDir INTEGER, sha256 TEXT, size INTEGER);");
        auto createTableQueryResult = m_database.exec(sqlCreateTableQuery);
        if (!createTableQueryResult) {
            katla::printError("Creating table failed!: {}", createTableQueryResult.error().message());
            return;
        }

        constexpr int BatchSize = 1024;
        const int fileHashListSize = flatList.size();

        for (int i = 0; i < flatList.size(); i += BatchSize) {

            auto beginTransactionResult = m_database.exec("BEGIN TRANSACTION;");
            if (!beginTransactionResult) {
                katla::printError("Error beginning transaction!: {}", beginTransactionResult.error().message());
                return;
            }

            for (int batchIndex = 0; batchIndex < BatchSize; batchIndex++) {
                if ((i + batchIndex) >= flatList.size()) {
                    break;
                }

                auto& entry = flatList[i + batchIndex];
                if (entry->relativePath.empty() || entry->size == 0) {
                    continue;
                }

                std::vector<std::pair<std::string, std::string>> values = {
                        {"relativePath", entry->relativePath},
                        {"isDir", entry->type == backer::FileSystemEntryType::Dir ? "1" : "0"},
                        {"sha256", Backer::formatHash(entry->hash)},
                        {"size", katla::format("{}", entry->size)}};

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

} // namespace backer