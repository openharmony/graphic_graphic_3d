/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SCENE_TEST_UTIL
#define SCENE_TEST_UTIL

#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>

SCENE_BEGIN_NAMESPACE()
namespace UTest {

inline bool CreateDirectories(CORE_NS::IFileManager& manager, BASE_NS::string_view path)
{
    // Verify that the target path exists. (and create missing ones)
    // Remove protocol.
    auto pos = path.find("://") + 3;
    for (;;) {
        size_t end = path.find('/', pos);
        auto part = path.substr(0, end);

        // The last "part" should be a file name, so terminate there.
        if (end == BASE_NS::string_view::npos) {
            break;
        }

        auto entry = manager.GetEntry(part);
        if (entry.type == entry.UNKNOWN) {
            manager.CreateDirectory(part);
        } else if (entry.type == entry.DIRECTORY) {
        } else if (entry.type == entry.FILE) {
            // Invalid path..
            return false;
        }
        pos = end + 1;
    }
    return true;
}

inline bool CopyFile(CORE_NS::IFileManager& manager, BASE_NS::string_view source, BASE_NS::string_view dest)
{
    auto s = manager.OpenFile(source);
    if (!s) {
        return false;
    }
    if (!CreateDirectories(manager, dest)) {
        return false;
    }
    auto d = manager.CreateFile(dest);
    if (!d) {
        return false;
    }
    constexpr size_t size = 65536;
    uint8_t buffer[size];
    size_t total = s->GetLength();
    while (total > 0) {
        auto todo = BASE_NS::Math::min(total, size);
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

} // namespace UTest
SCENE_END_NAMESPACE()

#endif // SCENE_TEST_UTIL
