/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: Base object implementation
 * Author: Lauri Jaaskela
 * Create: 2023-04-25
 */
#ifndef META_EXT_BASE_OBJECT_H
#define META_EXT_BASE_OBJECT_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

/// Simple base object implementation that has the most common interfaces for objects
class BaseObject : public IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle, IStaticMetadata> {
public:
    BaseObject() = default;
    ~BaseObject() override = default;

    // IObject
    //    BASE_NS::Uid GetClassId() const override;            //Must be implemented by derived class
    //    BASE_NS::string_view GetClassName() const override;   //Must be implemented by derived class

    InstanceId GetInstanceId() const override
    {
        return instanceId_;
    }
    BASE_NS::string GetName() const override
    {
        return GetInstanceId().ToString();
    }
    IObject::Ptr Resolve(const RefUri& uri) const override
    {
        return GetObjectRegistry().DefaultResolveObject(me_.lock(), uri);
    }

    IObject::Ptr GetSelf() const override
    {
        return me_.lock();
    }

    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return IntroduceInterfaces::GetInterfacesVector();
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

    // IDerived
    void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr&) override
    {
        me_ = interface_pointer_cast<IObjectInstance>(aggr);
    }

    // ILifecycle
    bool Build(const IMetadata::Ptr&) override
    {
        flags_ = GetObjectDefaultFlags();
        return true;
    }
    void SetInstanceId(InstanceId uid) override
    {
        instanceId_ = uid;
    }
    void Destroy() override
    {
        GetObjectRegistry().DisposeObject(instanceId_);
    }

    template<typename Interface>
    typename Interface::Ptr GetSelf() const
    {
        return interface_pointer_cast<Interface>(GetSelf());
    }

    // this allows to use the same macros for base objects too
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
    InstanceId instanceId_;
    IObjectInstance::WeakPtr me_;
    ObjectFlagBitsValue flags_ { ObjectFlagBits::DEFAULT_FLAGS };
};

META_END_NAMESPACE()

#endif
