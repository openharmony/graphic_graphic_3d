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

#ifndef API_BASE_UTIL_COMPILE_TIME_HASHES_H
#define API_BASE_UTIL_COMPILE_TIME_HASHES_H

#include <cstdint>

#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace CompileTime {
constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ull;
constexpr uint64_t FNV_PRIME = 1099511628211ull;

constexpr uint64_t lo(uint64_t x)
{
    return x & 0xFFFFFFFF;
}

constexpr uint64_t hi(uint64_t x)
{
    return x >> 32ull;
}

constexpr uint64_t mulu64(uint64_t a, uint64_t b)
{
    return 0 + (lo(a) * lo(b) & uint32_t(-1)) +
           (((((hi(lo(a) * lo(b)) + lo(a) * hi(b)) & uint32_t(-1)) + hi(a) * lo(b)) & uint32_t(-1)) << 32ull);
}

inline constexpr uint64_t FNV1aHash(const char* const first, uint64_t hash = FNV_OFFSET_BASIS)
{
    if (*first == 0) {
        return hash;
    }
    hash ^= (uint64_t)*first;
    // Using custom 64 bit multiply to silence "constant integer overflow" warning.
    hash = mulu64(hash, FNV_PRIME);
    return FNV1aHash(first + 1, hash);
}
} // namespace CompileTime
BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_COMPILE_TIME_HASHES_H