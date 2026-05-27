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
#include <cstring>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#define JSON_IMPL
#include <core/json/json.h>

#define JSON2_IMPL
#include <core/json/json2.h>

namespace {

// --- json v1 ---

void WalkValue(const CORE_NS::json::value& val, int depth)
{
    if (depth > 32) {
        return;
    }
    if (val.type == CORE_NS::json::type::object) {
        for (const auto& pair : val.object_) {
            (void)pair.key;
            WalkValue(pair.value, depth + 1);
        }
    } else if (val.type == CORE_NS::json::type::array) {
        for (const auto& elem : val.array_) {
            WalkValue(elem, depth + 1);
        }
    }
}

void FuzzJsonV1Readonly(const uint8_t* data, size_t size)
{
    BASE_NS::vector<char> buf(size + 1);
    if (memcpy_s(buf.data(), buf.size(), data, size) != EOK) {
        return;
    }
    buf[size] = '\0';

    auto value = CORE_NS::json::parse<CORE_NS::json::readonly_tag>(buf.data());
    if (value) {
        WalkValue(value, 0);
    }
}

void FuzzJsonV1Writable(const uint8_t* data, size_t size)
{
    BASE_NS::vector<char> buf(size + 1);
    if (memcpy_s(buf.data(), buf.size(), data, size) != EOK) {
        return;
    }
    buf[size] = '\0';

    auto value = CORE_NS::json::parse<CORE_NS::json::writable_tag>(buf.data());
    if (value) {
        auto str = CORE_NS::json::to_string(value);
        (void)str;
    }
}

// --- json v2 ---

void WalkValue2(const CORE_NS::json2::view& val, int depth)
{
    if (depth > 32) {
        return;
    }
    if (val.is_object()) {
        for (const auto& pair : val.as_object()) {
            (void)pair.escapedKey;
            WalkValue2(pair.value, depth + 1);
        }
    } else if (val.is_array()) {
        for (const auto& elem : val.as_array()) {
            WalkValue2(elem, depth + 1);
        }
    }
}

void FuzzJson2View(const uint8_t* data, size_t size)
{
    BASE_NS::string_view sv(reinterpret_cast<const char*>(data), size);
    auto value = CORE_NS::json2::view::parse(sv);
    if (value) {
        WalkValue2(value, 0);
    }
}

void FuzzJson2Value(const uint8_t* data, size_t size)
{
    BASE_NS::string_view sv(reinterpret_cast<const char*>(data), size);
    auto value = CORE_NS::json2::value::parse(sv);
    if (value) {
        auto str = value.to_string();
        (void)str;
    }
}

}  // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0 || size > 65536) {
        return 0;
    }

    FuzzJsonV1Readonly(data, size);
    FuzzJsonV1Writable(data, size);
    FuzzJson2View(data, size);
    FuzzJson2Value(data, size);
    return 0;
}
