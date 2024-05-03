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

#ifndef API_BASE_UTIL_HASH_H
#define API_BASE_UTIL_HASH_H

#include <cstdint>

#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ull;
constexpr uint64_t FNV_PRIME = 1099511628211ull;
constexpr uint32_t FNV_OFFSET_BASIS_32 = 0x811c9dc5u;
constexpr uint32_t FNV_PRIME_32 = 16777619u;

template<typename T>
uint64_t hash(const T& value);

template<>
inline uint64_t hash(const uint8_t& value)
{
    return value;
}
template<>
inline uint64_t hash(const uint16_t& value)
{
    return value;
}
template<>
inline uint64_t hash(const uint32_t& value)
{
    return value;
}
template<>
inline uint64_t hash(const uint64_t& value)
{
    return value;
}
#ifdef __APPLE__
template<>
inline uint64_t hash(const unsigned long& value)
{
    return value;
}
#endif
template<>
inline uint64_t hash(const int8_t& value)
{
    return static_cast<uint64_t>(value);
}
template<>
inline uint64_t hash(const int16_t& value)
{
    return static_cast<uint64_t>(value);
}
template<>
inline uint64_t hash(const int32_t& value)
{
    return static_cast<uint64_t>(value);
}
template<>
inline uint64_t hash(const int64_t& value)
{
    return static_cast<uint64_t>(value);
}
template<>
inline uint64_t hash(void* const& value)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(value));
}
template<typename T>
inline uint64_t hash(T* const& value)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(value));
}

inline uint32_t FNV1a32Hash(const uint8_t* data, size_t len)
{
    // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    uint32_t hash = FNV_OFFSET_BASIS_32;
    while (len--) {
        hash ^= *data;
        hash *= FNV_PRIME_32;
        ++data;
    }
    return hash;
}

template<class T>
inline uint32_t FNV1a32Hash(const T* data, size_t len)
{
    return FNV1a32Hash(reinterpret_cast<const uint8_t*>(data), len * sizeof(T));
}

template<class T>
inline uint32_t FNV1a32Hash(const T& data)
{
    return FNV1a32Hash(&reinterpret_cast<const uint8_t&>(data), sizeof(T));
}

inline uint64_t FNV1aHash(const uint8_t* data, size_t len)
{
    // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    uint64_t hash = FNV_OFFSET_BASIS;
    while (len--) {
        hash ^= *data;
        hash *= FNV_PRIME;
        ++data;
    }
    return hash;
}

template<class T>
inline uint64_t FNV1aHash(const T* data, size_t len)
{
    return FNV1aHash(reinterpret_cast<const uint8_t*>(data), len * sizeof(T));
}

template<class T>
inline uint64_t FNV1aHash(const T& data)
{
    return FNV1aHash(&reinterpret_cast<const uint8_t&>(data), sizeof(T));
}

template<typename T>
inline void HashCombine(uint64_t& seed, const T& v)
{
    constexpr const uint64_t GOLDEN_RATIO = 0x9e3779b9;
    constexpr const uint64_t ROTL = 6;
    constexpr const uint64_t ROTR = 2;
    seed ^= hash(v) + GOLDEN_RATIO + (seed << ROTL) + (seed >> ROTR);
}

template<typename... Rest>
inline void HashCombine(uint64_t& seed, const Rest&... rest)
{
    (HashCombine(seed, rest), ...);
}

template<typename... Rest>
inline uint64_t Hash(Rest&&... rest)
{
    uint64_t seed = 0;
    (HashCombine(seed, BASE_NS::forward<Rest>(rest)), ...);
    return seed;
}

template<typename Iter>
inline uint64_t HashRange(Iter first, Iter last)
{
    uint64_t seed = 0;
    for (; first != last; ++first) {
        HashCombine(seed, *first);
    }

    return seed;
}

template<typename Iter>
inline void HashRange(uint64_t& seed, Iter first, Iter last)
{
    for (; first != last; ++first) {
        HashCombine(seed, *first);
    }
}
BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_COMPILE_TIME_HASHES_H