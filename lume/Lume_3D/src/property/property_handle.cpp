/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include "property_handle.h"

#include <base/containers/type_traits.h>
#include <core/log.h>

CORE3D_BEGIN_NAMESPACE()
using CORE_NS::IPropertyApi;

PropertyHandle::PropertyHandle(IPropertyApi* owner, void* data, const size_t size) noexcept
    : IPropertyHandle(), owner_(owner), data_(data), size_(size)
{}

PropertyHandle::PropertyHandle(PropertyHandle&& other) noexcept
    : IPropertyHandle(), owner_(BASE_NS::exchange(other.owner_, nullptr)),
      data_(BASE_NS::exchange(other.data_, nullptr)), size_(BASE_NS::exchange(other.size_, 0))
{}

PropertyHandle& PropertyHandle::operator=(PropertyHandle&& other) noexcept
{
    if (&other != this) {
        owner_ = BASE_NS::exchange(other.owner_, nullptr);
        data_ = BASE_NS::exchange(other.data_, nullptr);
        size_ = BASE_NS::exchange(other.size_, 0);
    }
    return *this;
}

const IPropertyApi* PropertyHandle::Owner() const
{
    return owner_;
}

size_t PropertyHandle::Size() const
{
    return size_;
}

void* PropertyHandle::WLock()
{
    CORE_ASSERT(!locked_);
    locked_ = true;
    return data_;
}
void PropertyHandle::WUnlock()
{
    CORE_ASSERT(locked_);
    locked_ = false;
}

const void* PropertyHandle::RLock() const
{
    CORE_ASSERT(!locked_);
    locked_ = true;
    return data_;
}
void PropertyHandle::RUnlock() const
{
    CORE_ASSERT(locked_);
    locked_ = false;
}
CORE3D_END_NAMESPACE()
