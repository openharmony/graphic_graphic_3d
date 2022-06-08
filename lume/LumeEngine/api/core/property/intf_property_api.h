/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PROPERTY_IPROPERTY_API_H
#define API_CORE_PROPERTY_IPROPERTY_API_H

#include <cstddef>

#include <base/containers/array_view.h>
#include <core/namespace.h>
#include <core/property/property.h>

CORE_BEGIN_NAMESPACE()
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

    /** Creates a clone of the property data.
     * The memory buffer for data must be allocated before, but not "constructed". Clone method calls copy constructors
     * for the actual type. This method can be considered deprecated, and will be removed when JavaApi has been fixed.
     * @param dst Destination handle
     * @param src Source handle
     * @return Returns true if data was copied.
     */
    [[deprecated]] virtual bool Clone(IPropertyHandle& dst, const IPropertyHandle& src) const = 0;

    /** Destroys the property data.
        Does not free the handle or the memory buffer that handle points to. Calls the destructor for the actual type.
        The handle object must be destroyed after this.
        This method can be considered deprecated, and will be removed when JavaApi has been fixed.
     * @param dst Handle to Object instance to destroy. (should be "constructed" by the same owner)
     */
    [[deprecated]] virtual void Destroy(IPropertyHandle& dst) const = 0;

protected:
    IPropertyApi() = default;
    virtual ~IPropertyApi() = default;
    IPropertyApi(const IPropertyApi&) = delete;
    IPropertyApi(IPropertyApi&&) = delete;
    IPropertyApi& operator=(const IPropertyApi&) = delete;
};
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_IPROPERTY_API_H
