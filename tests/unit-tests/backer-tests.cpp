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

#include "gtest/gtest.h"

#include <gsl/span>

#include "katla/core/core.h"
#include "libbacker/backer.h"

#include <variant>

namespace backer {

    TEST(BackerTests, DuplicateFileTest) {
        auto hash1 = Backer::sha256(katla::format("{}/{}", CMAKE_SOURCE_DIR, "tests/test-sets/duplicate-test/dup1"));
        auto hash2 = Backer::sha256(katla::format("{}/{}", CMAKE_SOURCE_DIR, "tests/test-sets/duplicate-test/dup2"));

        ASSERT_TRUE(hash1 == hash2) << "Hashes not the same!";
    }

    TEST(BackerTests, NotDuplicateFileTest) {
        auto hash1 = Backer::sha256(katla::format("{}/{}", CMAKE_SOURCE_DIR, "tests/test-sets/duplicate-test/diff1"));
        auto hash2 = Backer::sha256(katla::format("{}/{}", CMAKE_SOURCE_DIR, "tests/test-sets/duplicate-test/diff2"));

        ASSERT_TRUE(hash1 != hash2) << "Hashes the same!";
    }

    TEST(BackerTests, GroupPotentialDuplicateFileTest) {
        auto fileGroupSet = FileGroupSet::create((katla::format("{}/{}", CMAKE_SOURCE_DIR, "tests/test-sets/group-potential-duplicates")));
        auto fileMap = fileGroupSet.fileMap();
        ASSERT_TRUE(fileMap.size() == 3) << "Expected 3 groups";
        ASSERT_TRUE(fileMap["diff1-6"].size() == 1);
        ASSERT_TRUE(fileMap["diff2-6"].size() == 1);
        ASSERT_TRUE(fileMap["dup-4"].size() == 2);
    }

    outcome::result<std::string> createTemporaryDir() {
        std::string dirTemplate = "/tmp/katla-test-XXXXXX";
        if (mkdtemp(dirTemplate.data()) == NULL) {
            return std::make_error_code(static_cast<std::errc>(errno));
        }
        return dirTemplate;
    }
}

