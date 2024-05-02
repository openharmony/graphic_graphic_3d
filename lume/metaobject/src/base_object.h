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
#ifndef META_SRC_BASE_OBJECT_H
#define META_SRC_BASE_OBJECT_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>

#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_type_info.h>

META_BEGIN_NAMESPACE()

namespace Internal {

class BaseObject : public IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle> {
protected:
    BaseObject() = default;
    ~BaseObject() override = default;

    // IObject
    //    BASE_NS::Uid GetClassId() const override;            //Must be implemented by derived class
    //    BASE_NS::string_view GetClassName() const override;   //Must be implemented by derived class
    InstanceId GetInstanceId() const override;
    BASE_NS::string GetName() const override;
    IObject::Ptr Resolve(const RefUri& uri) const override;
    IObject::Ptr GetSelf() const override;
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override;
    // IObjectFlags
    ObjectFlagBitsValue GetObjectFlags() const override;
    void SetObjectFlags(const ObjectFlagBitsValue& value) override;
    ObjectFlagBitsValue GetObjectDefaultFlags() const override;

    template<typename Interface>
    typename Interface::Ptr GetSelf() const
    {
        return interface_pointer_cast<Interface>(GetSelf());
    }

    // IDerived
    void SetSuperInstance(const IObject::Ptr&, const IObject::Ptr&) override;
    BASE_NS::Uid GetSuperClassUid() const override;

    // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;
    void SetInstanceId(InstanceId uid) override;
    void Destroy() override;

protected:
    IObjectRegistry& GetObjectRegistry() const;

protected:
    static StaticObjectMetadata& StaticObjectMeta()
    {
        static StaticObjectMetadata meta { META_NS::ClassId::BaseObject, nullptr };
        return meta;
    }

public:
    static const META_NS::StaticObjectMetadata& GetStaticObjectMetadata()
    {
        return StaticObjectMeta();
    }

private:
    InstanceId instanceId_;
    IObjectInstance::WeakPtr me_;
    ObjectFlagBitsValue flags_ { ObjectFlagBits::DEFAULT_FLAGS };
};

template<class FinalClass, const META_NS::ClassInfo& ClassInfo, class ConcreteBaseClass, class... Interfaces>
class ConcreteBaseFwd : public IntroduceInterfaces<Interfaces...>, public ConcreteBaseClass {
    STATIC_METADATA_MACHINERY(ClassInfo, ConcreteBaseClass)
    STATIC_INTERFACES_WITH_CONCRETE_BASE(IntroduceInterfaces<Interfaces...>, ConcreteBaseClass)
    META_DEFINE_OBJECT_TYPE_INFO(FinalClass, ClassInfo)
public:
    using ConcreteBaseClass::GetInterfacesVector;

protected:
    ObjectId GetClassId() const override
    {
        return ClassInfo.Id();
    }
    BASE_NS::string_view GetClassName() const override
    {
        return ClassInfo.Name();
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetStaticInterfaces();
    }

public:
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        auto* me = const_cast<ConcreteBaseFwd*>(this);
        return me->ConcreteBaseFwd::GetInterface(uid);
    }
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        CORE_NS::IInterface* ret = ConcreteBaseClass::GetInterface(uid);
        if (!ret) {
            ret = IntroduceInterfaces<Interfaces...>::GetInterface(uid);
        }
        return ret;
    }

protected:
    void Ref() override
    {
        ConcreteBaseClass::Ref();
    }
    void Unref() override
    {
        ConcreteBaseClass::Unref();
    }
};

template<class FinalClass, const META_NS::ClassInfo& ClassInfo, class... Interfaces>
class BaseObjectFwd : public ConcreteBaseFwd<FinalClass, ClassInfo, META_NS::Internal::BaseObject, Interfaces...> {
    using Impl = META_NS::Internal::BaseObject;

public:
    virtual IObjectRegistry& GetObjectRegistry() const
    {
        return BaseObject::GetObjectRegistry();
    }

protected:
    BaseObjectFwd() = default;
    ~BaseObjectFwd() override = default;
};

} // namespace Internal

class BaseObject : public Internal::BaseObjectFwd<BaseObject, META_NS::ClassId::BaseObject> {
    using Super = Internal::BaseObjectFwd<BaseObject, META_NS::ClassId::BaseObject>;

public:
    using Super::Super;
};

META_END_NAMESPACE()

#endif
