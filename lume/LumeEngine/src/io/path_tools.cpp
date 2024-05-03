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

#include "path_tools.h"

#include <cstdlib>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/log.h>
#include <core/namespace.h>

#include "util/string_util.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#elif defined(_WIN32)
#include <direct.h>
#endif

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

bool IsRelative(const string_view path)
{
    if (path.empty()) {
        return true;
    }
    return path[0] != '/';
}

bool ParseUri(string_view uri, string_view& protocol, string_view& path)
{
    const size_t index = uri.find(':');
    if (index != string_view::npos) {
        protocol = uri.substr(0, index);
        // remove scheme and separator
        uri.remove_prefix(index + 1U);
        // remove the authority separator if it's there
        if (uri.starts_with("//")) {
            uri.remove_prefix(2U);
        }
        path = uri;
        return true;
    }

    return false;
}

string NormalizePath(string_view path)
{
    if (path.empty()) {
        return "/";
    }
    string res;
    res.reserve(path.size());
    res.push_back('/');
    while (!path.empty()) {
        if (path[0] == '/') {
            path = path.substr(1);
            continue;
        }
        auto pos = path.find_first_of('/', 0);
        if (const string_view sub = path.substr(0, pos); sub == "..") {
            if ((!res.empty()) && (res.back() == '/')) {
                res.resize(res.size() - 1);
            }

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
        } else if (sub == ".") {
            path = path.substr(pos);
            continue;
        } else {
            res.append(sub);
        }
        if (pos == string::npos) {
            break;
        }
        res.push_back('/');
        path = path.substr(pos);
    }
    return res;
}

string GetCurrentDirectory()
{
    string basePath;
#if defined(__linux__) || defined(__APPLE__)
    // OSX and linux both implement the "null buf" extension which allocates the required amount of space.
    auto tmp = getcwd(nullptr, 0);
    if (tmp) {
        basePath = tmp;
        if (basePath.back() != '/') {
            basePath += '/';
        }
        free(tmp);
    } else {
        // fallback to root (either out-of-memory or the CWD is inaccessible for current user)
        basePath = "/";
        CORE_LOG_F("Could not get current working directory, initializing base path as '/'");
    }
#elif defined(_WIN32)
    // Windows also implements the "null buf" extension, but uses a different name and "format".
    auto tmp = _getcwd(nullptr, 0);
    if (tmp) {
        basePath = tmp;
        StringUtil::FindAndReplaceAll(basePath, "\\", "/");
        free(tmp);
    } else {
        // fallback to root (yes, technically it's the root of current drive, which again can change when ever.
        // but then again _getcwd should always work, except in out-of-memory cases where this is the least of our
        // problems.
        basePath = "/";
        CORE_LOG_F("Could not get current working directory, initializing base path as '/'");
    }
#else
    // Unsupported platform.fallback to root.
    basePath = "/";
#endif

    // Make sure that we end with a slash.
    if (basePath.back() != '/') {
        basePath.push_back('/');
    }
    // And make sure we start with a slash also.
    if (basePath.front() != '/') {
        basePath.insert(0, "/");
    }
    return basePath;
}

#if _WIN32
void SplitPath(string_view pathIn, string_view& drive, string_view& path, string_view& filename, string_view& ext)
{
    drive = path = filename = ext = {};
    if (pathIn[0] == '/') {
        // see if there is a drive after
        if (pathIn[2] == ':') { // 2: index of ':'
            // yes.
            // remove the first '/' to help later parsing
            pathIn = pathIn.substr(1);
        }
    }
    // extract the drive
    if (pathIn[1] == ':') {
        drive = pathIn.substr(0, 1);
        pathIn = pathIn.substr(2); // 2: remove the drive part
    }
    auto lastSlash = pathIn.find_last_of('/');
    if (lastSlash != string_view::npos) {
        filename = pathIn.substr(lastSlash + 1);
        path = pathIn.substr(0, lastSlash + 1);
    } else {
        filename = pathIn;
    }
    auto lastDot = filename.find_last_of('.');
    if (lastDot != string_view::npos) {
        ext = filename.substr(lastDot + 1);
        filename = filename.substr(0, lastDot);
    }
}
#endif
CORE_END_NAMESPACE()
