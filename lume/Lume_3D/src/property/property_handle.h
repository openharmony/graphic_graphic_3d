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

#ifndef CORE_PROPERTY_PROPERTY_HANDLE_H
#define CORE_PROPERTY_PROPERTY_HANDLE_H

#include <3d/namespace.h>
#include <core/property/intf_property_handle.h>

CORE3D_BEGIN_NAMESPACE()
// NOTE: this handle does not own the data at any point.
// it just stores the raw pointer given to it, and the owning property api.
class PropertyHandle : public CORE_NS::IPropertyHandle {
public:
    PropertyHandle() = delete;
    PropertyHandle(CORE_NS::IPropertyApi* owner, void* data, const size_t size) noexcept;
    PropertyHandle(const PropertyHandle& other) = delete;
    PropertyHandle(PropertyHandle&& other) noexcept;
    ~PropertyHandle() override = default;
    PropertyHandle& operator=(const PropertyHandle& other) = delete;
    PropertyHandle& operator=(PropertyHandle&& other) noexcept;

    const CORE_NS::IPropertyApi* Owner() const override;
    size_t Size() const override;

    void* WLock() override;
    const void* RLock() const override;
    void RUnlock() const override;
    void WUnlock() override;

protected:
    mutable bool locked_ { false };
    CORE_NS::IPropertyApi* owner_;
    void* data_;
    size_t size_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_PROPERTY_PROPERTY_HANDLE_H
