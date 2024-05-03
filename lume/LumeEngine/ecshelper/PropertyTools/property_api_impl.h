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

#ifndef CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_API_IMPL_H
#define CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_API_IMPL_H

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>

CORE_BEGIN_NAMESPACE()
struct Property;

template<typename BlockType>
class PropertyApiImpl : public IPropertyApi, protected IPropertyHandle {
public:
    PropertyApiImpl();
    PropertyApiImpl(BlockType* data, BASE_NS::array_view<const Property> props);
    ~PropertyApiImpl() override = default;

    IPropertyHandle* GetData();
    const IPropertyHandle* GetData() const;

    uint32_t GetGeneration() const;
    // IPropertyApi
    size_t PropertyCount() const override;
    const Property* MetaData(size_t index) const override;
    BASE_NS::array_view<const Property> MetaData() const override;
    IPropertyHandle* Create() const override;
    IPropertyHandle* Clone(const IPropertyHandle*) const override;
    void Release(IPropertyHandle*) const override;

protected:
    const IPropertyApi* Owner() const override;
    size_t Size() const override;
    const void* RLock() const override;
    void RUnlock() const override;
    void* WLock() override;
    void WUnlock() override;
    uint64_t Type() const override;

private:
    const IPropertyApi* owner_ { nullptr };
    BlockType* data_ { nullptr };
    BASE_NS::array_view<const Property> componentMetadata_;
    uint64_t typeHash_ { 0 };
    mutable uint32_t rLocked_ { 0 };
    uint32_t generationCount_ { 0 };
    mutable bool wLocked_ { false };
};
CORE_END_NAMESPACE()

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_API_IMPL_H
