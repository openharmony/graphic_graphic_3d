/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <new>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/namespace.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>
#include <core/property_tools/property_api_impl.h>

CORE_BEGIN_NAMESPACE()
namespace {
template<typename BlockType>
inline void ReleaseOwned(BlockType* data)
{
    delete data;
}

template<typename BlockType>
inline void ReleaseExternal(BlockType* data)
{}
} // namespace
template<typename BlockType>
PropertyApiImpl<BlockType>::PropertyApiImpl() = default;

template<typename BlockType>
PropertyApiImpl<BlockType>::PropertyApiImpl(BlockType* data, BASE_NS::array_view<const Property> aProps)
    : data_(data, ReleaseExternal), componentMetadata_(aProps)
{
    // Create a typeid by hashing the metadata information.
    typeHash_ = 0;
    for (auto& t : aProps) {
        BASE_NS::HashCombine(typeHash_, t.offset);
        BASE_NS::HashCombine(typeHash_, t.count);
        BASE_NS::HashCombine(typeHash_, t.type.compareHash);
        BASE_NS::HashCombine(typeHash_, t.type.typeHash);
        BASE_NS::HashCombine(typeHash_, t.hash);
        BASE_NS::HashCombine(typeHash_, t.size);
    }
}

template<typename BlockType>
size_t PropertyApiImpl<BlockType>::PropertyCount() const
{
    if (owner_) {
        return owner_->PropertyCount();
    }
    return componentMetadata_.size();
}

// Metadata for property
template<typename BlockType>
const Property* PropertyApiImpl<BlockType>::MetaData(size_t aIndex) const
{
    if (owner_) {
        return owner_->MetaData(aIndex);
    }
    return &componentMetadata_[aIndex];
}

// Metadata for properties
template<typename BlockType>
BASE_NS::array_view<const Property> PropertyApiImpl<BlockType>::MetaData() const
{
    if (owner_) {
        return owner_->MetaData();
    }
    return componentMetadata_;
}

template<typename BlockType>
IPropertyHandle* PropertyApiImpl<BlockType>::Create() const
{
    auto ret = new PropertyApiImpl<BlockType>();
    ret->owner_ = Owner();
    ret->typeHash_ = Type();
    ret->data_ =
        BASE_NS::unique_ptr<BlockType, void (*)(BlockType*)>(BASE_NS::make_unique<BlockType>().release(), ReleaseOwned);
    return ret;
}

template<typename BlockType>
IPropertyHandle* PropertyApiImpl<BlockType>::Clone(const IPropertyHandle* src) const
{
    if (src->Owner() == this) {
        auto* h = static_cast<const PropertyApiImpl<BlockType>*>(src);
        auto* ret = new PropertyApiImpl<BlockType>();
        ret->owner_ = Owner();
        ret->typeHash_ = Type();
        ret->data_ = BASE_NS::unique_ptr<BlockType, void (*)(BlockType*)>(
            BASE_NS::make_unique<BlockType>().release(), ReleaseOwned);
        *ret->data_ = *h->data_;
        return ret;
    }
    return nullptr;
}

template<typename BlockType>
void PropertyApiImpl<BlockType>::Release(IPropertyHandle* dst) const
{
    if (dst) {
        if (dst->Owner() == Owner()) {
            // we can only destroy things we "own" (know)
            auto* handle = static_cast<PropertyApiImpl<BlockType>*>(dst);
            if (handle->owner_) {
                // and only the ones that own the data..
                delete handle;
            }
        }
    }
}

template<typename BlockType>
uint32_t PropertyApiImpl<BlockType>::GetGeneration() const
{
    return generationCount_;
}

template<typename BlockType>
IPropertyHandle* PropertyApiImpl<BlockType>::GetData()
{
    return this;
}

template<typename BlockType>
const IPropertyHandle* PropertyApiImpl<BlockType>::GetData() const
{
    return this;
}

template<typename BlockType>
const IPropertyApi* PropertyApiImpl<BlockType>::Owner() const
{
    if (owner_) {
        return owner_;
    }
    return this;
}

template<typename BlockType>
size_t PropertyApiImpl<BlockType>::Size() const
{
    return sizeof(BlockType);
}

template<typename BlockType>
const void* PropertyApiImpl<BlockType>::RLock() const
{
#ifndef NDEBUG
    CORE_ASSERT(!wLocked_);
    rLocked_++;
#endif
    return data_.get();
}

template<typename BlockType>
void PropertyApiImpl<BlockType>::RUnlock() const
{
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ > 0);
    rLocked_--;
#endif
}

template<typename BlockType>
void* PropertyApiImpl<BlockType>::WLock()
{
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ <= 1 && !wLocked_);
    wLocked_ = true;
#endif
    return data_.get();
}

template<typename BlockType>
void PropertyApiImpl<BlockType>::WUnlock()
{
#ifndef NDEBUG
    CORE_ASSERT(wLocked_);
    wLocked_ = false;
#endif
    generationCount_++;
}

template<typename BlockType>
uint64_t PropertyApiImpl<BlockType>::Type() const
{
    return typeHash_;
}

template<>
inline size_t PropertyApiImpl<void>::Size() const
{
    return 0;
}

template<>
inline IPropertyHandle* PropertyApiImpl<void>::Create() const
{
    auto ret = new PropertyApiImpl<void>();
    ret->owner_ = owner_;
    ret->data_ = nullptr;
    return ret;
}

template<>
inline IPropertyHandle* PropertyApiImpl<void>::Clone(const IPropertyHandle* src) const
{
    if (src->Owner() == this) {
        auto* ret = new PropertyApiImpl<void>();
        ret->owner_ = owner_;
        return ret;
    }
    return nullptr;
}

template<>
inline void PropertyApiImpl<void>::Release(IPropertyHandle* dst) const
{
    if (dst) {
        if (dst->Owner() == this) {
            // we can only destroy things we "own" (know)
            auto* handle = static_cast<PropertyApiImpl<void>*>(dst);
            delete handle;
        }
    }
}

CORE_END_NAMESPACE()
