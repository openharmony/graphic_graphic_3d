/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

template<typename T>
uint64_t hash(const T& b);

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
    return value;
}
template<>
inline uint64_t hash(const int16_t& value)
{
    return value;
}
template<>
inline uint64_t hash(const int32_t& value)
{
    return value;
}
template<>
inline uint64_t hash(const int64_t& value)
{
    return value;
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

inline uint64_t FNV1aHash(const uint8_t* aData, size_t aLen, uint64_t hash = CompileTime::FNV_OFFSET_BASIS)
{
    // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    const uint8_t* src = aData;
    while (aLen--) {
        hash ^= *src;
        hash *= CompileTime::FNV_PRIME;
        src++;
    }
    return hash;
}

template<class T>
inline uint64_t FNV1aHash(const T* aData, size_t aLen, uint64_t hash = CompileTime::FNV_OFFSET_BASIS)
{
    return FNV1aHash(reinterpret_cast<const uint8_t*>(aData), aLen * sizeof(T), hash);
}

template<class T>
inline uint64_t FNV1aHash(const T& aData, uint64_t hash = CompileTime::FNV_OFFSET_BASIS)
{
    return FNV1aHash(&reinterpret_cast<const uint8_t&>(aData), sizeof(T), hash);
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
    size_t seed = 0;
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