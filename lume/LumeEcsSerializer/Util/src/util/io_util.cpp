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

#include "util/io_util.h"

#include <charconv>

#include <base/math/mathf.h>

#include <core/io/intf_file_manager.h>
#include <core/json/json.h>
#include <core/intf_engine.h>

#include "util/path_util.h"
#include "util/json_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

UTIL_BEGIN_NAMESPACE()

namespace IoUtil {

bool WriteCompatibilityInfo(json::standalone_value& jsonOut, const CompatibilityInfo& info)
{
    jsonOut["compatibility_info"] = json::standalone_value::object();
    jsonOut["compatibility_info"]["version"] =
        string(to_string(info.versionMajor) + "." + to_string(info.versionMinor));
    jsonOut["compatibility_info"]["type"] = string(info.type);
    return true;
}

SerializationResult CheckCompatibility(const json::value& json, array_view<CompatibilityRange const> validVersions)
{
    SerializationResult result;

    string type;
    string version;
    if (const json::value* iter = json.find("compatibility_info"); iter) {
        string parseError;
        SafeGetJsonValue(*iter, "type", parseError, type);
        SafeGetJsonValue(*iter, "version", parseError, version);

        uint32_t versionMajor{ 0 };
        uint32_t versionMinor{ 0 };
        if (const auto delim = version.find('.'); delim != string::npos) {
            std::from_chars(version.data(), version.data() + delim, versionMajor);
            const size_t minorStart = delim + 1;
            std::from_chars(version.data() + minorStart, version.data() + version.size(), versionMinor);
        } else {
            std::from_chars(version.data(), version.data() + version.size(), versionMajor);
        }

        result.compatibilityInfo.versionMajor = versionMajor;
        result.compatibilityInfo.versionMinor = versionMinor;
        result.compatibilityInfo.type = type;

        for (const auto& range : validVersions) {
            if (type != range.type) {
                continue;
            }
            if ((range.versionMajorMin != CompatibilityRange::IGNORE_VERSION) &&
                (versionMajor < range.versionMajorMin)) {
                continue;
            }
            if ((range.versionMajorMax != CompatibilityRange::IGNORE_VERSION) &&
                (versionMajor > range.versionMajorMax)) {
                continue;
            }
            if ((range.versionMinorMin != CompatibilityRange::IGNORE_VERSION) &&
                (versionMinor < range.versionMinorMin)) {
                continue;
            }
            if ((range.versionMinorMax != CompatibilityRange::IGNORE_VERSION) &&
                (versionMinor > range.versionMinorMax)) {
                continue;
            }

            // A compatible version was found from the list of valid versions.
            return result;
        }
    }

    // Not a compatible version.
    result.status = Status::ERROR_COMPATIBILITY_MISMATCH;
    result.error = "Unsupported version. type: '" + type + "' version: '" + version + "'";
    return result;
}

bool FileExists(CORE_NS::IFileManager& fileManager, BASE_NS::string_view folder, BASE_NS::string_view file)
{
    if (auto dir = fileManager.OpenDirectory(folder)) {
        auto entries = dir->GetEntries();
        for (const auto& entry : entries) {
            if (entry.type == CORE_NS::IDirectory::Entry::FILE && entry.name == file) {
                return true;
            }
        }
    }
    return false;
}

bool CreateDirectories(CORE_NS::IFileManager& fileManager, string_view pathUri)
{
    // Verify that the target path exists. (and create missing ones)
    // Remove protocol.
    auto pos = pathUri.find("://") + 3;
    for (;;) {
        size_t end = pathUri.find('/', pos);
        auto part = pathUri.substr(0, end);

        // The last "part" should be a file name, so terminate there.
        if (end == string_view::npos) {
            break;
        }

        auto entry = fileManager.GetEntry(part);
        if (entry.type == entry.UNKNOWN) {
            fileManager.CreateDirectory(part);
        } else if (entry.type == entry.DIRECTORY) {
        } else if (entry.type == entry.FILE) {
            // Invalid path..
            return false;
        }
        pos = end + 1;
    }
    return true;
}

bool DeleteDirectory(CORE_NS::IFileManager& fileManager, string_view pathUri)
{
    auto dir = fileManager.OpenDirectory(pathUri);
    if (!dir) {
        return false;
    }

    bool result = true;

    for (auto& entry : dir->GetEntries()) {
        auto childUri = PathUtil::ResolvePath(pathUri, entry.name);
        switch (entry.type) {
            case IDirectory::Entry::Type::FILE:
                result = fileManager.DeleteFile(childUri) && result;
                break;
            case IDirectory::Entry::Type::DIRECTORY:
                result = DeleteDirectory(fileManager, childUri) && result;
                break;
            default:
                // NOTE: currently unknown type is just ignored and does not affect the result.
                break;
        }
    }

    // Result is true if all copy operations succeeded.
    return fileManager.DeleteDirectory(pathUri) && result;
}

bool Copy(CORE_NS::IFileManager& fileManager, string_view sourceUri, string_view destinationUri)
{
    bool destinationIsDir = false;
    string tmp; // Just to keep the string alive for this function scope.

    // If the destination is a directory. copy the source into that dir.
    {
        auto destinationDir = fileManager.OpenDirectory(destinationUri);
        if (destinationDir) {
            destinationIsDir = true;
            auto filename = PathUtil::GetFilename(sourceUri);
            tmp = PathUtil::ResolvePath(destinationUri, filename);
            destinationUri = tmp;
        }
    }

    // First try copying as a file.
    if (CopyFile(fileManager, sourceUri, destinationUri)) {
        return true;
    }

    // Then try copying as a dir (if the destination is a dir).
    if (destinationIsDir) {
        auto destinationUriAsDir = destinationUri + "/";
        CreateDirectories(fileManager, destinationUriAsDir);
        return CopyDirectoryContents(fileManager, sourceUri, destinationUriAsDir);
    }

    return false;
}

bool Move(CORE_NS::IFileManager& fileManager, string_view sourceUri, string_view destinationUri)
{
    return fileManager.Rename(sourceUri, destinationUri);
}

bool CopyFile(CORE_NS::IFileManager& fileManager, string_view sourceUri, string_view destinationUri)
{
    auto s = fileManager.OpenFile(sourceUri);
    if (!s) {
        return false;
    }
    if (!CreateDirectories(fileManager, destinationUri)) {
        return false;
    }
    auto d = fileManager.CreateFile(destinationUri);
    if (!d) {
        return false;
    }
    constexpr size_t bufferSize = 65536; // copy in 64kb blocks
    uint8_t buffer[bufferSize];
    size_t total = s->GetLength();
    while (total > 0) {
        auto todo = Math::min(total, bufferSize);
        if (todo != s->Read(buffer, todo)) {
            return false;
        }
        if (todo != d->Write(buffer, todo)) {
            return false;
        }
        total -= todo;
    }
    return true;
}

bool CopyDirectoryContents(CORE_NS::IFileManager& fileManager, string_view sourceUri, string_view destinationUri)
{
    auto dst = fileManager.OpenDirectory(destinationUri);
    if (!dst) {
        return false;
    }

    auto src = fileManager.OpenDirectory(sourceUri);
    if (!src) {
        return false;
    }

    bool result = true;

    for (auto& entry : src->GetEntries()) {
        auto childSrc = PathUtil::ResolvePath(sourceUri, entry.name);
        auto childDst = PathUtil::ResolvePath(destinationUri, entry.name);

        switch (entry.type) {
            case IDirectory::Entry::Type::FILE:
                result = CopyFile(fileManager, childSrc, childDst) && result;
                break;
            case IDirectory::Entry::Type::DIRECTORY:
                if (!fileManager.CreateDirectory(childDst)) {
                    result = false;
                    continue;
                }
                result = CopyDirectoryContents(fileManager, childSrc, childDst) && result;
                break;
            default:
                // NOTE: currently unknown type is just ignored and does not affect the result.
                break;
        }
    }

    // Result is true if all copy operations succeeded.
    return result;
}

bool SaveTextFile(CORE_NS::IFileManager& fileManager, string_view fileUri, string_view fileContents)
{
    // TODO: the safest way to save files would be to save to a temp file and rename.
    auto file = fileManager.CreateFile(fileUri);
    if (file) {
        file->Write(fileContents.data(), fileContents.length());
        file->Close();
        return true;
    }

    return false;
}

bool LoadTextFile(CORE_NS::IFileManager& fileManager, string_view fileUri, string& fileContentsOut)
{
    auto file = fileManager.OpenFile(fileUri);
    if (file) {
        const size_t length = file->GetLength();
        fileContentsOut.resize(length);
        return file->Read(fileContentsOut.data(), length) == length;
    }
    return false;
}

template<typename Work>
void ReplaceTextInFilesImpl(CORE_NS::IFileManager& fileManager, BASE_NS::string_view folderUri, Work& replace)
{
    if (auto dir = fileManager.OpenDirectory(folderUri)) {
        auto entries = dir->GetEntries();
        for (const auto& entry : entries) {
            if (entry.type == CORE_NS::IDirectory::Entry::FILE) {
                auto separator_pos = entry.name.find_last_of(".");
                auto ending = entry.name.substr(separator_pos);
                bool isPlaintext { false };
                static BASE_NS::vector<BASE_NS::string_view> plaintextTypes { ".txt", ".cpp", ".h", ".json", ".cmake" };
                for (const auto& type : plaintextTypes) {
                    if (ending == type) {
                        isPlaintext = true;
                    }
                }
                // TODO: odds of getting a match in a binary just by chance seem pretty slim so perhaps this check
                // could be omitted, but I suppose that depends on the length of the tag we're replacing
                if (isPlaintext) {
                    auto inFilePath = PathUtil::ResolvePath(folderUri, entry.name);
                    ReplaceTextInFileImpl(fileManager, inFilePath, replace);
                }
            } else if (entry.type == CORE_NS::IDirectory::Entry::DIRECTORY) {
                auto path = PathUtil::ResolvePath(folderUri, entry.name);
                ReplaceTextInFilesImpl(fileManager, path, replace);
            }
        }
    }
}

template<typename Work>
void ReplaceTextInFileImpl(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri, Work& replace)
{
    auto inFile = fileManager.OpenFile(uri);
    if (inFile) {
        BASE_NS::string stringContents;
        size_t len = inFile->GetLength();
        stringContents.resize(len);
        inFile->Read(stringContents.data(), len);

        replace(stringContents);

        auto dataToWrite = stringContents.data();
        auto lenToWrite = stringContents.length();
        fileManager.DeleteFile(uri);
        auto outFile = fileManager.CreateFile(uri);
        if (outFile) {
            outFile->Write(dataToWrite, lenToWrite);
        }
    }
}

void ReplaceTextInFiles(CORE_NS::IFileManager& fileManager, BASE_NS::string_view folderUri, BASE_NS::string_view text,
    BASE_NS::string_view replaceWith)
{
    auto replace = [&text, &replaceWith](BASE_NS::string& stringContents) {
        auto pos = stringContents.find(text, 0UL);
        while (pos != BASE_NS::string::npos) {
            stringContents = stringContents.replace(
                stringContents.begin() + static_cast<int64_t>(pos), stringContents.begin() + static_cast<int64_t>(pos + text.size()), replaceWith);
            pos += replaceWith.size();
            pos = stringContents.find(text, pos);
        }
    };
    ReplaceTextInFilesImpl(fileManager, folderUri, replace);
}

void ReplaceTextInFiles(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view folderUri, BASE_NS::vector<Replacement> replacements)
{
    auto replace = [&replacements](BASE_NS::string& stringContents) {
        for (const auto& repl : replacements) {
            auto pos = stringContents.find(repl.from, 0UL);
            while (pos != BASE_NS::string::npos) {
                stringContents = stringContents.replace(
                    stringContents.begin() + static_cast<int64_t>(pos), stringContents.begin() + static_cast<int64_t>(pos + repl.from.size()), repl.to);
                pos += repl.to.size();
                pos = stringContents.find(repl.from, pos);
            }
        }
    };
    ReplaceTextInFilesImpl(fileManager, folderUri, replace);
}

void ReplaceTextInFile(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri, BASE_NS::vector<Replacement> replacements)
{
    auto replace = [&replacements](BASE_NS::string& stringContents) {
        for (const auto& repl : replacements) {
            auto pos = stringContents.find(repl.from, 0UL);
            while (pos != BASE_NS::string::npos) {
                stringContents = stringContents.replace(stringContents.begin() + static_cast<int64_t>(pos),
                    stringContents.begin() + static_cast<int64_t>(pos + repl.from.size()), repl.to);
                pos += repl.to.size();
                pos = stringContents.find(repl.from, pos);
            }
        }
    };
    ReplaceTextInFileImpl(fileManager, uri, replace);
}

bool CopyAndRenameFiles(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fromUri, BASE_NS::string_view toUri,
    BASE_NS::string_view filename)
{
    auto template_dir = fileManager.OpenDirectory(fromUri);
    auto project_dir = fileManager.OpenDirectory(toUri);
    bool result{ true };
    // copy the .cpp and .h behavior files from the template to the project, renaming them
    if (template_dir && project_dir) {
        for (const auto& entry : template_dir->GetEntries()) {
            if (entry.type != CORE_NS::IDirectory::Entry::Type::FILE) {
                continue;
            }
            const auto& n = entry.name;
            auto ending = n.substr(n.find_last_of('.'));
            auto from = PathUtil::ResolvePath(fromUri, n);
            auto to = PathUtil ::ResolvePath(toUri, filename + ending);
            if (!CopyFile(fileManager, from, to)) {
                result = false;
                break;
            }
        }
    } else {
        result = false;
    }
    return result;
}

template<typename Work>
void InsertInFileBoilerplate(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, Work& inner)
{
    if (auto inFile = fileManager.OpenFile(fileUri)) {
        BASE_NS::string stringContents;
        size_t len = inFile->GetLength();
        stringContents.resize(len);
        inFile->Read(stringContents.data(), len);

        inner(stringContents, len);

        auto dataToWrite = stringContents.data();
        auto lenToWrite = stringContents.length();
        fileManager.DeleteFile(fileUri);
        auto outFile = fileManager.CreateFile(fileUri);
        outFile->Write(dataToWrite, lenToWrite);
    }
}

void InsertIntoString(
    BASE_NS::string& search, BASE_NS::string& insertion, InsertType type, BASE_NS::string& stringContents, size_t len)
{
    auto pos = stringContents.find(search, 0UL);
    if (pos != BASE_NS::string::npos) {
        if (type == InsertType::TAG) {
            auto nlPos = stringContents.find("\n", pos);
            if (nlPos == BASE_NS::string::npos) {
                nlPos = len - 1;
            }
            stringContents.insert(nlPos + 1, (insertion + "\r\n").data());
        } else if (type == InsertType::SIGNATURE) {
            auto bPos = stringContents.find('{', pos);
            if (bPos == BASE_NS::string::npos) {
                return;
            }
            size_t depth{ 0 };
            auto endPos = BASE_NS::string::npos;
            while (bPos < len) {
                const auto& ch = stringContents[bPos];
                if (ch == '}') {
                    depth--;
                    if (depth == 0) {
                        endPos = bPos;
                        break;
                    }
                } else if (ch == '{') {
                    depth++;
                }
                bPos++;
            }
            if (endPos != BASE_NS::string::npos) {
                if (endPos == len - 1) {
                    stringContents.insert(len - 1, ("\r\n" + insertion + "\r\n").data());
                } else {
                    stringContents.insert(endPos, (insertion + "\r\n").data());
                }
            }
        }
    }
}

void InsertInFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::string search,
    BASE_NS::string insertion, InsertType type)
{
    auto func = [&search, &insertion, &type](BASE_NS::string& stringContents, size_t len) {
        InsertIntoString(search, insertion, type, stringContents, len);
    };
    InsertInFileBoilerplate(fileManager, fileUri, func);
}

void InsertInFile(
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view fileUri, BASE_NS::vector<Insertion> insertions)
{
    auto func = [&insertions](BASE_NS::string& stringContents, size_t len) {
        for (auto& ins : insertions) {
            InsertIntoString(ins.searchStr, ins.insertStr, ins.type, stringContents, len);
        }
    };
    InsertInFileBoilerplate(fileManager, fileUri, func);
};

void ReplaceInString(BASE_NS::string& string, const BASE_NS::string& target, const BASE_NS::string& replacement)
{
    auto pos = string.find(target, 0UL);
    while (pos != BASE_NS::string::npos) {
        string = string.replace(string.begin() + static_cast<int64_t>(pos), string.begin() + static_cast<int64_t>(pos + target.size()), replacement);
        pos += replacement.size();
        pos = string.find(target, pos);
    }
}

} // namespace IoUtil

UTIL_END_NAMESPACE()
