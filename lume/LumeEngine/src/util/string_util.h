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

#ifndef CORE_UTIL_STRING_UTIL_H
#define CORE_UTIL_STRING_UTIL_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
namespace StringUtil {
template<class T, size_t N>
constexpr size_t MaxStringLengthFromArray(T (&)[N])
{
    return N - 1u;
}

inline void CopyStringToArray(const BASE_NS::string_view source, char* target, size_t maxLength)
{
    if (source.size() > maxLength) {
        CORE_LOG_W("CopyStringToArray: string (%zu) longer than %zu", source.size(), maxLength);
    }
    size_t const length = source.copy(target, maxLength);
    target[length] = '\0';
}

inline bool NotSpace(unsigned char ch)
{
    return !std::isspace(static_cast<int>(ch));
}

// trim from start (in place)
inline void LTrim(BASE_NS::string_view& string)
{
    auto const count = size_t(std::find_if(string.begin(), string.end(), NotSpace) - string.begin());
    string.remove_prefix(count);
}

// trim from end (in place)
inline void RTrim(BASE_NS::string_view& string)
{
    auto const count =
        size_t(std::distance(std::find_if(string.rbegin(), string.rend(), NotSpace).base(), string.end()));
    string.remove_suffix(count);
}

// trim from both ends (in place)
inline size_t Trim(BASE_NS::string_view& string)
{
    RTrim(string);
    LTrim(string);
    return string.length();
}

inline BASE_NS::vector<BASE_NS::string_view> Split(
    const BASE_NS::string_view string, const BASE_NS::string_view delims = "|")
{
    BASE_NS::vector<BASE_NS::string_view> output;
    auto left = string;

    while (!left.empty()) {
        auto const pos = left.find_first_of(delims);

        auto found = left.substr(0, pos);
        if (Trim(found) > 0) {
            output.push_back(found);
        }
        if (pos != BASE_NS::string_view::npos) {
            left.remove_prefix(pos + 1);
        } else {
            break;
        }
    }

    return output;
}

// find and replace first instance of "find" with "replace" in "source"
inline bool FindAndReplaceOne(
    BASE_NS::string& source, const BASE_NS::string_view find, const BASE_NS::string_view replace)
{
    const auto p = source.find(find);
    if (p != BASE_NS::string::npos) {
        source.replace(source.cbegin() + static_cast<BASE_NS::string::difference_type>(p),
            source.cbegin() + static_cast<BASE_NS::string::difference_type>(p + find.length()), replace);
    }
    return (p != BASE_NS::string::npos);
}
// find and replace all instances of "find" with "replace" in "source"
inline void FindAndReplaceAll(
    BASE_NS::string& source, const BASE_NS::string_view find, const BASE_NS::string_view replace)
{
    while (FindAndReplaceOne(source, find, replace))
        ;
}
} // namespace StringUtil
CORE_END_NAMESPACE()

#endif // CORE_UTIL_STRING_UTIL_H
