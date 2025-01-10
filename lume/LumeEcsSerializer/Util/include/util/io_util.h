/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef API_UTIL_IOUTIL_H
#define API_UTIL_IOUTIL_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>

#include <core/json/json.h>
#include <core/io/intf_file_manager.h>

#include <util/namespace.h>

UTIL_BEGIN_NAMESPACE()

namespace IoUtil {

enum class Status : uint32_t {
    SUCCESS = 0,
    ERROR_GENERAL = 1,
    ERROR_COMPATIBILITY_MISMATCH = 3,
};

struct CompatibilityInfo {
    uint32_t versionMajor{ 0 };
    uint32_t versionMinor{ 0 };
    BASE_NS::string type;
};

struct CompatibilityRange {
    static const uint32_t IGNORE_VERSION = { ~0u };

    uint32_t versionMajorMin{ IGNORE_VERSION };
    uint32_t versionMajorMax{ IGNORE_VERSION };
    uint32_t versionMinorMin{ IGNORE_VERSION };
    uint32_t versionMinorMax{ IGNORE_VERSION };
    BASE_NS::string type{};
};

struct SerializationResult {
    Status status{ Status::SUCCESS };
    CompatibilityInfo compatibilityInfo{};
    BASE_NS::string error{};

    operator bool()
    {
        return status == Status::SUCCESS;
    }
};

bool WriteCompatibilityInfo(CORE_NS::json::standalone_value& jsonOut, const CompatibilityInfo& info);
SerializationResult CheckCompatibility(
    const CORE_NS::json::value& json, BASE_NS::array_view<CompatibilityRange const> validVersions);

bool CreateDirectories(CORE_NS::IFileManager& fileManager, BASE_NS::string_view pathUri);
bool DeleteDirectory(CORE_NS::IFileManager& fileManager, BASE_NS::string_view directoryUri);

bool Copy(CORE_NS::IFileManager& fileManager, BASE_NS::string_view sourceUri, BASE_NS::string_view destinationUri);
bool Move(CORE_NS::IFileManager& fileManager, BASE_NS::string_view sourceUri, BASE_NS::string_view destinationUri);
bool CopyFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view sourceUri, BASE_NS::string_view destinationUri);
bool CopyDirectoryContents(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view sourceUri, BASE_NS::string_view destinationUri);

bool SaveTextFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::string_view fileContents);
bool LoadTextFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::string& fileContentsOut);

// Copy files from fromUri dir to toUri dir, renaming the files to filename (excluding the filetype suffix)
// e.g. template.h, template.cpp -> filename.h, filename.cpp
bool CopyAndRenameFiles(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fromUri, BASE_NS::string_view toUri,
    BASE_NS::string_view filename);

// Replace all instances of text dir's files with replaceWith, recursively
// only affects .h .cpp .txt and .json files
void ReplaceTextInFiles(CORE_NS::IFileManager& fileManager, BASE_NS::string_view dir, BASE_NS::string_view text,
    BASE_NS::string_view replaceWith);

struct Replacement {
    BASE_NS::string from;
    BASE_NS::string to;
};
// A version of ReplaceTextInFiles that does multiple replacements while the file is open
void ReplaceTextInFiles(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view folderUri, BASE_NS::vector<Replacement> replacements);

void ReplaceTextInFile(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri, BASE_NS::vector<Replacement> replacements);

enum class InsertType : char {
    TAG,      // Finds the first instance of the tag string in the file and inserts a newline and insertion after it
    SIGNATURE // Finds signature string in file, seeks next '{' character,
    // and inserts a new line and the insertion string before the matching closing '}'
};
struct Insertion {
    BASE_NS::string searchStr;
    BASE_NS::string insertStr;
    InsertType type;
};
void InsertInFile(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::vector<Insertion> insertions);
void InsertInFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::string search,
    BASE_NS::string insertion, InsertType type);
void ReplaceInString(BASE_NS::string& string, const BASE_NS::string& target, const BASE_NS::string& replacement);

} // namespace IoUtil

UTIL_END_NAMESPACE()

#endif // API_UTIL_IOUTIL_H
