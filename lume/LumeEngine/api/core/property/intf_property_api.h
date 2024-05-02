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

#ifndef API_CORE_PROPERTY_IPROPERTY_API_H
#define API_CORE_PROPERTY_IPROPERTY_API_H

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct Property;
class IPropertyHandle;

/** @ingroup group_property_ipropertyapi */
/** IProperty api */
class IPropertyApi {
public:
    /** Property count of component.
     */
    virtual size_t PropertyCount() const = 0;

    /** Metadata for property.
     */
    virtual const Property* MetaData(size_t index) const = 0;

    /** Metadata for properties.
     */
    virtual BASE_NS::array_view<const Property> MetaData() const = 0;

    /** Create a new handle (default initialized data)
     */
    virtual IPropertyHandle* Create() const = 0;

    /** Create a new handle with a copy of srcHandle data
     */
    virtual IPropertyHandle* Clone(const IPropertyHandle* srcHandle) const = 0;

    /** Fully destruct the handle. The handle will be completely released and no calls to it shall be made anymore.
     */
    virtual void Release(IPropertyHandle* handle) const = 0;

    /* Type hash, can be used to see if the data is of matching type, without comparing the MetaDatas.
     */
    virtual uint64_t Type() const = 0;

protected:
    IPropertyApi() = default;
    virtual ~IPropertyApi() = default;
    IPropertyApi(const IPropertyApi&) = delete;
    IPropertyApi(IPropertyApi&&) = delete;
    IPropertyApi& operator=(const IPropertyApi&) = delete;
};
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_IPROPERTY_API_H
