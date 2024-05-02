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

#ifndef API_BASE_CONTAINERS_ALLOCATOR_H
#define API_BASE_CONTAINERS_ALLOCATOR_H

#include <cstdint>
#include <cstdlib> // malloc, free
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
    if (dst && src && srcSize <= dstSize) {
        // Note: arguments for memcpy have been verified.
        auto status = memcpy_s(dst, dstSize, src, srcSize);
        if (status != 0) {
            return false;
        }
    } else {
        BASE_LOG_E("CloneData invalid arguments.");
    }
    return (dst && src && srcSize <= dstSize);
}

inline bool MoveData(void* const dst, const size_t dstSize, const void* const src, const size_t srcSize)
{
    if (dst && src && srcSize <= dstSize) {
        // Note: arguments for memmove have been verified.
        auto status = memmove_s(dst, dstSize, src, srcSize);
        if (status != 0) {
            return false;
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
    auto status = memset_s(dst, dstSize, val, count);
    if (status != 0) {
        return false;
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
