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

#include "jsontyped_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/json/json2.h>
#include <core/namespace.h>

// FDP-driven target for the typed JSON API: derives typed parameters and grammar-biased strings
// from the raw bytes, which the plain byte-buffer targets cannot.

namespace {
constexpr const char HEX[] = "0123456789abcdefABCDEF";

// \uXXXX, surrogate pairs, named escapes and plain bytes, biased toward the decoder's
// surrogate / codepoint branches.
std::string BuildUnicodeString(FuzzedDataProvider& fdp)
{
    std::string out;
    while (fdp.remaining_bytes() > 0) {
        switch (fdp.ConsumeIntegral<uint8_t>() % 6) {
            case 0:
                out += static_cast<char>(fdp.ConsumeIntegral<uint8_t>());
                break;
            case 1:
                out += "\\u";
                for (int j = 0; j < 4 && fdp.remaining_bytes() > 0; ++j) {
                    out += HEX[fdp.ConsumeIntegral<uint8_t>() % 22];
                }
                break;
            case 2:
                out += '\\';
                out += static_cast<char>(fdp.ConsumeIntegral<uint8_t>());
                break;
            case 3:  // high surrogate followed by low surrogate
                out += "\\ud8";
                out += HEX[fdp.ConsumeIntegral<uint8_t>() % 16];
                out += HEX[fdp.ConsumeIntegral<uint8_t>() % 16];
                out += "\\udc";
                out += HEX[fdp.ConsumeIntegral<uint8_t>() % 16];
                out += HEX[fdp.ConsumeIntegral<uint8_t>() % 16];
                break;
            case 4:  // truncated \u with 0-3 hex digits
                out += "\\u";
                for (size_t n = fdp.ConsumeIntegral<uint8_t>() % 4; n > 0; --n) {
                    out += HEX[fdp.ConsumeIntegral<uint8_t>() % 22];
                }
                break;
            default:
                out += "\\n";
                break;
        }
    }
    return out;
}

// Multi-byte UTF-8 lead/continuation sequences, including truncated ones.
std::string BuildUtf8String(FuzzedDataProvider& fdp)
{
    std::string out;
    while (fdp.remaining_bytes() > 0) {
        uint8_t lead = fdp.ConsumeIntegral<uint8_t>();
        out += static_cast<char>(lead);
        int cont = 0;
        if (lead >= 0xF0) {
            cont = 3;
        } else if (lead >= 0xE0) {
            cont = 2;
        } else if (lead >= 0xC0) {
            cont = 1;
        }
        for (int j = 0; j < cont && fdp.remaining_bytes() > 0; ++j) {
            out += static_cast<char>(fdp.ConsumeIntegral<uint8_t>());
        }
    }
    return out;
}

void FuzzAccessors(FuzzedDataProvider& fdp)
{
    std::string src = fdp.ConsumeRemainingBytesAsString();
    BASE_NS::string holder(src.data(), src.size());  // kept alive: readonly values reference it

    if (auto value = CORE_NS::json::parse(holder.c_str()); value) {
        (void)value.as_number<int64_t>();
        (void)value.as_number<uint64_t>();
        (void)value.as_number<double>();
        if (value.is_object()) {
            (void)value.find("key");
            (void)value.find("");
            (void)value["key"];  // inserts when absent
            (void)value.find("missing");
        }
        CORE_NS::json::standalone_value owned(value);  // cross-tag deep copy
        (void)CORE_NS::json::to_string(owned);
    }

    // json2: parse is length-aware (string_view), json2::view is the read-only alias.
    auto view = CORE_NS::json2::parse(BASE_NS::string_view(holder.data(), holder.size()));
    if (!view.is_uninitialized()) {
        if (view.is_number()) {
            (void)view.as_number<int64_t>();
            (void)view.as_number<uint64_t>();
            (void)view.as_number<double>();
        }
        if (view.is_object()) {
            (void)view.find("key");
            (void)view.find("missing");
        }
        if (view.is_string()) {
            (void)view.as_string();
        }
        (void)CORE_NS::json2::to_string(view);
    }
}

void FuzzConstructors(FuzzedDataProvider& fdp)
{
    switch (fdp.ConsumeIntegral<uint8_t>() % 5) {
        case 0: {
            std::string s = fdp.ConsumeRemainingBytesAsString();
            BASE_NS::string_view sv(s.data(), s.size());
            (void)CORE_NS::json::to_string(CORE_NS::json::value(sv));
            (void)CORE_NS::json2::to_string(CORE_NS::json2::value::string(sv));
            break;
        }
        case 1: {
            int64_t n = fdp.ConsumeIntegral<int64_t>();
            (void)CORE_NS::json::to_string(CORE_NS::json::value(n));
            (void)CORE_NS::json2::to_string(CORE_NS::json2::value::number(n));
            break;
        }
        case 2: {
            double d = fdp.ConsumeFloatingPoint<double>();
            (void)CORE_NS::json::to_string(CORE_NS::json::value(d));
            (void)CORE_NS::json2::to_string(CORE_NS::json2::value::number(d));
            break;
        }
        case 3: {
            bool b = fdp.ConsumeBool();
            (void)CORE_NS::json::to_string(CORE_NS::json::value(b));
            (void)CORE_NS::json2::to_string(CORE_NS::json2::value::boolean(b));
            break;
        }
        default: {
            (void)CORE_NS::json::to_string(CORE_NS::json::value(CORE_NS::json::value::null{}));
            (void)CORE_NS::json2::to_string(CORE_NS::json2::value::null());
            break;
        }
    }
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    switch (fdp.ConsumeIntegral<uint8_t>() % 4) {
        case 0: {
            std::string generated = BuildUnicodeString(fdp);
            BASE_NS::string_view sv(generated.data(), generated.size());
            (void)CORE_NS::json::unescape(sv);
            (void)CORE_NS::json::escape(sv);
            (void)CORE_NS::json2::escaped_string(CORE_NS::json2::escaped_string_view(sv)).value();
            (void)CORE_NS::json2::escape(sv);
            break;
        }
        case 1: {
            std::string generated = BuildUtf8String(fdp);
            BASE_NS::string_view sv(generated.data(), generated.size());
            (void)CORE_NS::json::escape(sv);
            (void)CORE_NS::json::unescape(sv);
            (void)CORE_NS::json2::escape(sv);
            (void)CORE_NS::json2::escaped_string(CORE_NS::json2::escaped_string_view(sv)).value();
            break;
        }
        case 2:
            FuzzAccessors(fdp);
            break;
        default:
            FuzzConstructors(fdp);
            break;
    }
    return 0;
}
