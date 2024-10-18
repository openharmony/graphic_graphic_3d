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

#include "util/path_util.h"

#include <base/math/mathf.h>

using namespace BASE_NS;

UTIL_BEGIN_NAMESPACE()

namespace PathUtil {

string NormalizePath(string_view path)
{
    string res;
    if (path[0] != '/') {
        res.reserve(path.size() + 1);
        res.push_back('/');
    } else {
        res.reserve(path.size());
    }
    while (!path.empty()) {
        if (path[0] == '/') {
            if (res.empty()) {
                res.push_back('/');
            }
            path = path.substr(1);
            continue;
        }
        auto pos = path.find_first_of("/", 0);
        string_view sub = path.substr(0, pos);
        if (sub == ".") {
            path = path.substr(pos);
            continue;
        } else if (sub == "..") {
            if ((!res.empty()) && (res.back() == '/')) {
                res.resize(res.size() - 1);
            }
            // chop_last_segment(res);
            if (auto p = res.find_last_of('/'); string::npos != p) {
                res.resize(p);
            } else {
                if (res.empty()) {
                    // trying to back out of root. (ie. invalid path)
                    return "";
                }
                res.clear();
            }
            if (pos == string::npos) {
                res.push_back('/');
                break;
            }
        } else {
            res.append(sub);
        }
        if (pos == string::npos) {
            break;
        } else {
            res.push_back('/');
        }
        path = path.substr(pos);
    }
    if (res[0] != '/') {
        res.insert(0, "/");
    }
    return res;
}

string GetParentPath(string_view path)
{
    if (path.size() > 1 && path[path.size() - 1] == '/' && path[path.size() - 2] != '/') {
        // Allow (ignore) trailing '/' for folders.
        path = path.substr(0, path.size() - 1);
    }

    const size_t separatorPos = path.rfind('/');
    if (separatorPos == string::npos) {
        return "";
    } else {
        return string(path.substr(0, separatorPos + 1));
    }
}

string ResolvePath(string_view parent, string_view uri, bool allowQueryString)
{
    size_t queryPos = allowQueryString ? string::npos : uri.find('?');
    string_view path = (string::npos != queryPos) ? uri.substr(0, queryPos) : uri;

    if (parent.empty()) {
        return string(path);
    }

    if (path.empty()) {
        return string(parent);
    }

    if (path[0] == '/') {
        path = path.substr(1);
    } else if (path.find("://") != path.npos) {
        return string(path);
    }

    // NOTE: Resolve always assumes the parent path is a directory even if there is no '/' in the end
    if (parent.back() == '/') {
        return parent + path;
    } else {
        return parent + "/" + path;
    }
}

string GetRelativePath(string_view path, string_view relativeTo)
{
    // remove the common prefix
    {
        int lastSeparator = -1;

        for (size_t i = 0, iMax = Math::min(path.size(), relativeTo.size()); i < iMax; ++i) {
            if (path[i] != relativeTo[i]) {
                break;
            }
            if (path[i] == '/') {
                lastSeparator = int(i);
            }
        }

        const auto lastPos = static_cast<size_t>(static_cast<size_t>(lastSeparator) + 1);
        path.remove_prefix(lastPos);
        relativeTo.remove_prefix(lastPos);
    }

    if (path[1] == ':' && relativeTo[1] == ':' && path[0] != relativeTo[0]) {
        // files are on different drives
        return string(path);
    }

    // count and remove the directories left in relative_to
    auto directoriesCount = 0;
    {
        auto nextSeparator = relativeTo.find('/');
        while (nextSeparator != string::npos) {
            relativeTo.remove_prefix(nextSeparator + 1);
            nextSeparator = relativeTo.find('/');
            ++directoriesCount;
        }
    }

    string relativePath = "";
    for (auto i = 0, iMax = directoriesCount; i < iMax; ++i) {
        relativePath.append("../");
    }

    relativePath.append(path);

    return relativePath;
}

string GetFilename(string_view path)
{
    if (!path.empty() && path[path.size() - 1] == '/') {
        // Return a name also for folders.
        path = path.substr(0, path.size() - 1);
    }

    size_t cutPos = path.find_last_of("\\/");
    if (string::npos != cutPos) {
        return string(path.substr(cutPos + 1));
    } else {
        return string(path);
    }
}

string GetExtension(string_view path)
{
    size_t fileExtCut = path.rfind('.');
    if (fileExtCut != string::npos) {
        size_t queryCut = path.find('?', fileExtCut);
        if (queryCut != string::npos) {
            return string(path.substr(fileExtCut + 1, queryCut));
        } else {
            return string(path.substr(fileExtCut + 1, queryCut));
        }
    }
    return "";
}

string GetBaseName(string_view path)
{
    auto filename = GetFilename(path);
    size_t fileExtCut = filename.rfind(".");
    if (string::npos != fileExtCut) {
        filename.erase(fileExtCut);
    }
    return filename;
}

unordered_map<string, string> GetUriParameters(string_view uri)
{
    const size_t queryPos = uri.find('?');
    if (queryPos != string::npos) {
        unordered_map<string, string> params;
        size_t paramStartPos = queryPos;
        while (paramStartPos < uri.size()) {
            size_t paramValuePos = uri.find('=', paramStartPos + 1);
            size_t paramEndPos = uri.find('&', paramStartPos + 1);
            if (paramEndPos == string::npos) {
                paramEndPos = uri.size();
            }
            if (paramValuePos != string::npos && paramValuePos < paramEndPos) {
                auto key = uri.substr(paramStartPos + 1, paramValuePos - paramStartPos - 1);
                auto value = uri.substr(paramValuePos + 1, paramEndPos - paramValuePos - 1);
                params[key] = value;
            } else {
                auto key = uri.substr(paramStartPos + 1, paramEndPos - paramStartPos - 1);
                params[key] = key;
            }
            paramStartPos = paramEndPos;
        }
        return params;
    }
    return {};
}

string ResolveUri(string_view contextUri, string_view uri, bool allowQueryString)
{
    return ResolvePath(GetParentPath(contextUri), uri, allowQueryString);
}

} // namespace PathUtil

UTIL_END_NAMESPACE()
