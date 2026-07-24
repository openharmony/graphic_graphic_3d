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

#include "jsonescape_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/json/json2.h>
#include <core/namespace.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    BASE_NS::string_view input(reinterpret_cast<const char*>(data), size);
    // Oracle unescape(escape(x)) == x holds only for ASCII: escape re-encodes bytes >= 0x80 as
    // \uXXXX and is lossy on invalid UTF-8, so guard to 0x00-0x7F where the round-trip is exact.
    bool ascii = true;
    for (auto c : input) {
        if (static_cast<uint8_t>(c)= 0x80) {
            ascii = false;
            break;
        }
    }
    {
        BASE_NS::string escaped = CORE_NS::json::escape(input);

        BASE_NS::string appended = "prefix";  // in-place overload appends
        CORE_NS::json::escape(appended, input);

        if (ascii) {
            BASE_NS::string roundTrip = CORE_NS::json::unescape(BASE_NS::string_view(escaped.data(), escaped.size()));
            if (BASE_NS::string_view(roundTrip.data(), roundTrip.size()) != input) {
                abort();
            }
        }
    }
    {
        CORE_NS::json2::escaped_string escaped = CORE_NS::json2::escape(input);

        if (ascii) {
            BASE_NS::string roundTrip = escaped.value();
            if (BASE_NS::string_view(roundTrip.data(), roundTrip.size()) != input) {
                abort();
            }
        }
    }
    return 0;
}
