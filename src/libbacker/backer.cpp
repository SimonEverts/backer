#include "backer.h"

#include "core/core.h"
#include "katla/core/posix-file.h"

#include <filesystem>
#include <openssl/sha.h>

namespace backer {

    namespace fs = std::filesystem;

    std::vector<std::byte> Backer::sha256(std::string path) {

        katla::printInfo("sha: {}", path);

        katla::PosixFile file;
        auto openResult = file.open(path, katla::PosixFile::OpenFlags::ReadOnly);
        if (!openResult) {
            throw new std::runtime_error("Failed opening file for reading md5!");
        }

        constexpr int readBufferSize = 32768;
        std::vector<std::byte> readBuffer(readBufferSize);

        SHA256_CTX ctx;
        if (!SHA256_Init(&ctx)) {
            throw std::runtime_error("Failed initiaizing md5 context!");
        }

        auto fileSizeResult = file.size();
        if (!fileSizeResult) {
            throw std::runtime_error("Failed reading file size!");
        }

        int nrOfIter = (fileSizeResult.value() / readBufferSize) + 1;
        for (int i = 0; i < nrOfIter; i++) {
            auto readSpan = gsl::span<std::byte>(readBuffer.data(), readBuffer.size());
            auto readResult = file.read(readSpan);

            if (!readResult) {
                throw std::runtime_error("Failed reading file!");
            }

            SHA256_Update(&ctx, reinterpret_cast<const unsigned char *>(readBuffer.data()), readResult.value());
        }

        std::vector<std::byte> result(SHA256_DIGEST_LENGTH);
        SHA256_Final(reinterpret_cast<unsigned char *>(result.data()), &ctx);

        return result;
    }

    std::vector<std::byte> Backer::sha256(std::vector<std::vector<std::byte>> hashes) {

        SHA256_CTX ctx;
        if (!SHA256_Init(&ctx)) {
            throw new std::runtime_error("Failed initiaizing md5 context!");
        }

        for (auto& hash : hashes) {
            SHA256_Update(&ctx, reinterpret_cast<const unsigned char *>(hash.data()), hash.size());
        }

        std::vector<std::byte> result(SHA256_DIGEST_LENGTH);
        SHA256_Final(reinterpret_cast<unsigned char *>(result.data()), &ctx);

        return result;
    }

    std::vector<std::byte> Backer::sha256FromString(std::string string) {

        SHA256_CTX ctx;
        if (!SHA256_Init(&ctx)) {
            throw new std::runtime_error("Failed initiaizing md5 context!");
        }

        SHA256_Update(&ctx, reinterpret_cast<const unsigned char *>(string.data()), string.size());

        std::vector<std::byte> result(SHA256_DIGEST_LENGTH);
        SHA256_Final(reinterpret_cast<unsigned char *>(result.data()), &ctx);

        return result;
    }

    std::string Backer::formatHash(const std::vector<std::byte> &hash) {
        std::stringstream ss;

        for (auto o : hash) {
            ss << katla::format("{:02X}", o);
        }

        return ss.str();
    }

    std::vector<std::byte> Backer::parseHashString(const std::string& hashString) {
        std::vector<std::byte> bytes;

        for (unsigned int i = 0; i < hashString.length(); i += 2) {
            std::string byteString = hashString.substr(i, 2);
            std::byte byte = static_cast<std::byte>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }

        return bytes;
    }

    void Backer::writeToFile(std::string filePath, std::map<std::string, FileSystemEntry> &fileData) {
        katla::PosixFile file;
        auto result = file.create(filePath,
                                  katla::PosixFile::OpenFlags::Create | katla::PosixFile::OpenFlags::Truncate |
                                  katla::PosixFile::OpenFlags::WriteOnly);
        if (!result) {
            throw std::runtime_error(result.error().message());
        }

        for (auto &pair : fileData) {
            std::string path = katla::format("{}\n", pair.first);

            gsl::span <std::byte> s(reinterpret_cast<std::byte *>(path.data()), path.size());
            auto writeResult = file.write(s);
            if (!writeResult) {
                katla::print(stderr, "Failed writing to file: {}\n", filePath);
            }
        }

        auto closeResult = file.close();
        if (!closeResult) {
            katla::print(stderr, "Failed closing file: {}\n", filePath);
        }
    }

    void Backer::writeToFile(std::string filePath, std::vector<std::string> output) {
        katla::PosixFile file;
        auto result = file.create(filePath,
                                  katla::PosixFile::OpenFlags::Create | katla::PosixFile::OpenFlags::Truncate |
                                  katla::PosixFile::OpenFlags::WriteOnly);
        if (!result) {
            throw std::runtime_error(result.error().message());
        }

        for (auto &it : output) {
            std::string line = katla::format("{}\n", it);

            gsl::span <std::byte> s(reinterpret_cast<std::byte *>(line.data()), line.size());
            auto writeResult = file.write(s);
            if (!writeResult) {
                katla::print(stderr, "Failed writing to file: {}\n", filePath);
            }
        }

        auto closeResult = file.close();
        if (!closeResult) {
            katla::print(stderr, "Failed closing file: {}\n", filePath);
        }
    }

} // namespace backer