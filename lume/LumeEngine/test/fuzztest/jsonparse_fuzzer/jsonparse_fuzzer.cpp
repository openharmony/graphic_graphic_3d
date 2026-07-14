/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "jsonparse_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include <base/containers/string.h>
#include <core/json/json.h>
#include <core/json/json2.h>
#include <core/namespace.h>

// No round-trip oracle: parse stores the raw escaped slice and to_string re-escapes it, so the
// round-trip is not value-preserving once a string is present.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // parse() is null-terminated; an embedded NUL truncates the input (no length-aware overload).
    BASE_NS::string input(reinterpret_cast<const char*>(data), size);
    {
        auto value = CORE_NS::json::parse(input.c_str());
        if (value) {
            BASE_NS::string out;
            CORE_NS::json::to_string(out, value);
            (void)CORE_NS::json::to_string(value);
        }
    }
    {
        auto value = CORE_NS::json2::parse(input);
        if (value) {
            BASE_NS::string out;
            CORE_NS::json2::to_string(out, value);
            (void)CORE_NS::json2::to_string(value);
        }
    }
    return 0;
}
