/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <string>

#include <base/containers/string.h>
#include <base/containers/string_view.h>

namespace {

// Reproduce the path parsing logic used by the legacy scene importer's
// FindObject() function. The parser handles separators '.', '/', '!',
// escape sequences with '\\', and array subscripts '[n]'.
//
// This fuzzer exercises the parsing logic directly to find edge cases
// in separator detection, escape handling, and index extraction.

constexpr char SEPARATOR_PROPERTY = '.';
constexpr char SEPARATOR_CONTAINER = '/';
constexpr char SEPARATOR_ATTACHMENT = '!';
constexpr char ESCAPE_CHAR = '\\';

// Finds the first unescaped separator in a path string.
// Returns the position or string_view::npos if not found.
size_t FindFirstSeparator(BASE_NS::string_view path)
{
    bool escaped = false;
    for (size_t i = 0; i < path.size(); ++i) {
        if (escaped) {
            escaped = false;
            continue;
        }
        if (path[i] == ESCAPE_CHAR) {
            escaped = true;
            continue;
        }
        if (path[i] == SEPARATOR_PROPERTY || path[i] == SEPARATOR_CONTAINER || path[i] == SEPARATOR_ATTACHMENT) {
            return i;
        }
    }
    return BASE_NS::string_view::npos;
}

// Extracts array index from a name like "foo[42]".
// Returns the name part and the index (-1 if no subscript).
void GetNameAndSub(BASE_NS::string_view token, BASE_NS::string_view& name, int& index)
{
    index = -1;
    auto bracket = token.find('[');
    if (bracket == BASE_NS::string_view::npos) {
        name = token;
        return;
    }
    name = token.substr(0, bracket);
    auto end = token.find(']', bracket);
    if (end != BASE_NS::string_view::npos && end > bracket + 1) {
        auto numStr = token.substr(bracket + 1, end - bracket - 1);
        int val = 0;
        for (auto c : numStr) {
            if (c >= '0' && c <= '9') {
                val = val * 10 + (c - '0');
            } else {
                val = -1;
                break;
            }
        }
        index = val;
    }
}

void FuzzPathParsing(const uint8_t* data, size_t size)
{
    BASE_NS::string_view path(reinterpret_cast<const char*>(data), size);

    // Exercise separator finding across the full path
    size_t pos = 0;
    int iterations = 0;
    constexpr int MAX_ITERATIONS = 1000;
    while (pos < path.size() && iterations < MAX_ITERATIONS) {
        auto remaining = path.substr(pos);
        auto sepPos = FindFirstSeparator(remaining);
        if (sepPos == BASE_NS::string_view::npos) {
            // Last token — extract name and index
            BASE_NS::string_view name;
            int index = -1;
            GetNameAndSub(remaining, name, index);
            (void)name;
            (void)index;
            break;
        }

        // Extract token before separator
        auto token = remaining.substr(0, sepPos);
        BASE_NS::string_view name;
        int index = -1;
        GetNameAndSub(token, name, index);
        (void)name;
        (void)index;

        // Skip separator
        pos += sepPos + 1;
        iterations++;
    }
}

}  // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0 || size > 4096) {
        return 0;
    }

    FuzzPathParsing(data, size);
    return 0;
}
