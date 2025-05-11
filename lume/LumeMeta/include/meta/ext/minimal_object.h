/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Minimal object implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-08-08
 */

#ifndef META_EXT_MINIMAL_OBJECT_H
#define META_EXT_MINIMAL_OBJECT_H

#include <base/util/uid_util.h>

#include <meta/base/namespace.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_static_metadata.h>

META_BEGIN_NAMESPACE()

/// Simple object implementation that implements minimal set of interfaces for object
class MinimalObject : public IntroduceInterfaces<IObject, IObjectFlags, IStaticMetadata> {
public:
    MinimalObject() : flags_(ObjectFlagBits::DEFAULT_FLAGS) {}
    MinimalObject(const ObjectFlagBitsValue& value) : flags_(value) {}

    BASE_NS::string GetName() const override
    {
        return GetClassId().ToString();
    }

    ObjectFlagBitsValue GetObjectFlags() const override
    {
        return flags_;
    }

    void SetObjectFlags(const ObjectFlagBitsValue& value) override
    {
        flags_ = value;
    }

    ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        return ObjectFlagBits::DEFAULT_FLAGS;
    }

    // this allows to use the same macros for minimal objects too
    inline static const nullptr_t STATIC_METADATA {};
    static const StaticObjectMetadata* StaticMetadata()
    {
        return nullptr;
    }
    const StaticObjectMetadata* GetStaticMetadata() const override
    {
        return StaticMetadata();
    }

private:
    ObjectFlagBitsValue flags_ {};
};

META_END_NAMESPACE()

#endif
