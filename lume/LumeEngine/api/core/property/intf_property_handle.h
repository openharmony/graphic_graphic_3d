/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PROPERTY_IPROPERTY_HANDLE_H
#define API_CORE_PROPERTY_IPROPERTY_HANDLE_H

#include <cstddef>

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IPropertyApi;

/** @ingroup group_property_ipropertyhandle */
/** IProperty handle */
class IPropertyHandle {
public:
    /** Return the IPropertyApi implementation used to create the handle.
     */
    virtual const IPropertyApi* Owner() const = 0;

    /** Return size of this handle.
     */
    virtual size_t Size() const = 0;

    /** Return a read-only data of this handle.
     */
    virtual const void* RLock() const = 0;

    /** Release the read lock
     */
    virtual void RUnlock() const = 0;

    /** Return a modifiable data of this handle.
     */
    virtual void* WLock() = 0;

    /** Release the write lock
     */
    virtual void WUnlock() = 0;

protected:
    IPropertyHandle() = default;
    IPropertyHandle(const IPropertyHandle&) = delete;
    IPropertyHandle(IPropertyHandle&&) = delete;
    IPropertyHandle& operator=(const IPropertyHandle&) = delete;
    virtual ~IPropertyHandle() = default;
};
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_IPROPERTY_HANDLE_H
