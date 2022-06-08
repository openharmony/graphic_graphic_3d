/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_UTIL_LINEAR_ALLOCATOR_H
#define CORE_UTIL_LINEAR_ALLOCATOR_H

#include <cstdlib>
#include <memory>

#include <base/containers/unique_ptr.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class LinearAllocator {
public:
    explicit LinearAllocator(size_t size) : size_(size), data_(BASE_NS::make_unique<uint8_t[]>(size))
    {
        Reset();
    }

    LinearAllocator(size_t size, size_t alignment)
        : size_(size), alignment_(alignment), data_(BASE_NS::make_unique<uint8_t[]>(size_ + alignment_ - 1))
    {
        Reset();
    }

    ~LinearAllocator() = default;

    template<typename T>
    T* Allocate()
    {
        return static_cast<T*>(Allocate(sizeof(T), alignof(T)));
    }

    void* Allocate(size_t size)
    {
        return Allocate(size, 1);
    }

    void* Allocate(size_t size, size_t alignment)
    {
        if (std::align(alignment, size, current_, free_)) {
            void* result = current_;

            current_ = static_cast<char*>(current_) + size;
            free_ -= size;
            return result;
        }

        return nullptr;
    }

    inline void Reset()
    {
        current_ = data_.get();
        free_ = size_;
        auto allocationSize = size_ + alignment_ - 1;
        std::align(alignment_, 1, current_, allocationSize);
    }

    inline size_t GetByteSize() const
    {
        return size_;
    }

    inline size_t GetCurrentByteSize() const
    {
        return size_ - free_;
    }

    inline size_t GetAlignment() const
    {
        return alignment_;
    }

private:
    size_t const size_ { 0 };
    size_t const alignment_ = 1;
    size_t free_ { 0 };

    BASE_NS::unique_ptr<uint8_t[]> data_;
    void* current_ { nullptr };
};
CORE_END_NAMESPACE()

#endif // CORE_UTIL_LINEAR_ALLOCATOR_H
