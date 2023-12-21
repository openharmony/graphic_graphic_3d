/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
CORE_BEGIN_NAMESPACE()
#if _MSC_VER
// visual studio 2017 does not handle [[deprecated]] properly.
// so disable deprecation warning for this declaration.
// it will still properly warn in use.
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

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

    bool Clone(IPropertyHandle&, const IPropertyHandle&) const override;
    void Destroy(IPropertyHandle&) const override;

protected:
    const IPropertyApi* owner_ { nullptr };
    BlockType* data_ { nullptr };
    BASE_NS::array_view<const Property> componentMetadata_;
    const IPropertyApi* Owner() const override;
    size_t Size() const override;
    const void* RLock() const override;
    void RUnlock() const override;
    void* WLock() override;
    void WUnlock() override;
    uint64_t Type() const override;
    uint64_t typeHash_ { 0 };
    mutable uint32_t rLocked_ { 0 };
    uint32_t generationCount_ { 0 };
    mutable bool wLocked_ { false };
};
#if _MSC_VER
// revert to old warnings. (re-enables the deprecation warning)
#pragma warning(pop)
#endif
CORE_END_NAMESPACE()

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_API_IMPL_H
