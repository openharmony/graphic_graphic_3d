/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_CONTAINERS_ALLOCATOR_H
#define API_BASE_CONTAINERS_ALLOCATOR_H

#include <cstddef>
#include <cstdint>

#if !defined(BASE_HAS_ENGINE)
#include <malloc.h> // malloc, free
#endif

#include <securec.h>

#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
struct allocator {
    using size_type = size_t;
    void* instance { nullptr };
    void* (*alloc)(void* instance, size_type size) { nullptr };
    void (*free)(void* instance, void* ptr) { nullptr };
};

inline bool CloneData(void* const dst, const size_t dstSize, const void* const src, const size_t srcSize)
{
    BASE_ASSERT(src && dst && dstSize >= srcSize);
    if (dst && src && srcSize <= dstSize) {
        // Note: arguments for memcpy have been verified.
        if (memcpy_s(dst, dstSize, src, srcSize) != 0) {
            BASE_LOG_E("CloneData: memcpy failed");
        }
    } else {
        BASE_LOG_E("CloneData invalid arguments.");
    }
    return (dst && src && srcSize <= dstSize);
}

inline bool MoveData(void* const dst, const size_t dstSize, const void* const src, const size_t srcSize)
{
    BASE_ASSERT(src && dst && dstSize >= srcSize);
    if (dst && src && srcSize <= dstSize) {
        // Note: arguments for memmove have been verified.
        if (memmove_s(dst, dstSize, src, srcSize) != 0) {
            BASE_LOG_E("MoveData: memmove failed");
        }
    } else {
        BASE_LOG_E("MoveData invalid arguments.");
    }
    return (dst && src && srcSize <= dstSize);
}

inline bool ClearToValue(void* dst, size_t dstSize, uint8_t val, size_t count)
{
    if (dst == nullptr) {
        BASE_LOG_E("ClearToValue invalid arguments");
        return false;
    } else if (count > dstSize) {
        count = dstSize;
    }
    // Note: arguments for memset have been verified.
    if (memset_s(dst, dstSize, val, count) != 0) {
        BASE_LOG_E("ClearToValue: memset failed");
    }
    return true;
}

inline allocator& default_allocator()
{
    static allocator DefaultAllocInstance { nullptr,
        [](void* instance, allocator::size_type size) -> void* { return ::malloc(size); },
        [](void* instance, void* aPtr) { ::free(aPtr); } };
    return DefaultAllocInstance;
}
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_ALLOCATOR_H
