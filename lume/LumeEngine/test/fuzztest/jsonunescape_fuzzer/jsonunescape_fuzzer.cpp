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

#include "jsonunescape_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/json/json2.h>
#include <core/namespace.h>

// No oracle: unescape is lossy and not invertible.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    BASE_NS::string_view input(reinterpret_cast<const char*>(data), size);
    {
        BASE_NS::string unescaped = CORE_NS::json::unescape(input);

        BASE_NS::string appended = "prefix";  // in-place overload appends
        CORE_NS::json::unescape(appended, input);
    }
    {
        BASE_NS::string unescaped = CORE_NS::json2::escaped_string(CORE_NS::json2::escaped_string_view(input)).value();
    }
    return 0;
}
