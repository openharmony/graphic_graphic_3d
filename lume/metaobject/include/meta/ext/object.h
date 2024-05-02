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

#ifndef META_EXT_OBJECT_H
#define META_EXT_OBJECT_H

#include <base/util/uid_util.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/static_object_metadata.h>

#include "object_factory.h"

META_BEGIN_NAMESPACE()
namespace Internal {
// dummy pure virtual to shutup compiler warnings
class DummyGetObjectRegistry {
    virtual META_NS::IObjectRegistry& GetObjectRegistry() = 0;
};
} // namespace Internal

/**
 * @brief A helper template class for implementing a class which implements a basic set of object interfaces.
 * @note Usually when inheriting from this template directly SuperClassInfo should be META_NS::ClassId::BaseObject.
 */
template<class FinalClass, const META_NS::ClassInfo& ClassInfo, const META_NS::ClassInfo& SuperClassInfo,
    class... Interfaces>
class BaseObjectFwd : public IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle, Interfaces...>,
                      private Internal::DummyGetObjectRegistry {
    using Fwd = BaseObjectFwd;
    using Super = IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle, Interfaces...>;

public:
    // using declaration doesn't work with vc, still thinks the function access is protected
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        return Super::GetInterface(uid);
    }
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return Super::GetInterface(uid);
    }
    template<typename Type>
    constexpr Type* GetInterface() noexcept
    {
        return static_cast<Type*>(Super::StaticGetInterface(Type::UID));
    }
    template<typename Type>
    constexpr const Type* GetInterface() const noexcept
    {
        auto* me = const_cast<BaseObjectFwd*>(this);
        return static_cast<const Type*>(me->StaticGetInterface(Type::UID));
    }

public:
    META_DEFINE_OBJECT_TYPE_INFO(FinalClass, ClassInfo)

public:
    META_NS::IObjectRegistry& GetObjectRegistry() override
    {
        return META_NS::GetObjectRegistry();
    }

    static const StaticObjectMetadata& GetStaticObjectMetadata()
    {
        return StaticObjectMeta();
    }

public: // IObject
    BASE_NS::string_view GetClassName() const override
    {
        return ClassInfo.Name();
    }
    InstanceId GetInstanceId() const override
    {
        return object_->GetInstanceId();
    }
    ObjectId GetClassId() const override
    {
        return ClassInfo.Id();
    }
    BASE_NS::string GetName() const override
    {
        return object_->GetName();
    }
    IObject::Ptr Resolve(const RefUri& uri) const override
    {
        return object_->Resolve(uri);
    }
    template<typename Interface>
    typename Interface::Ptr Resolve(const RefUri& uri) const
    {
        return interface_pointer_cast<Interface>(object_->Resolve(uri));
    }
    IObject::Ptr GetSelf() const override
    {
        return object_ ? object_->GetSelf() : nullptr;
    }
    template<typename Interface>
    typename Interface::Ptr GetSelf() const
    {
        return interface_pointer_cast<Interface>(GetSelf());
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetStaticInterfaces();
    }

    static BASE_NS::vector<BASE_NS::Uid> GetStaticInterfaces()
    {
        return Super::GetInterfacesVector();
    }

public: // IObjectFlags
    ObjectFlagBitsValue GetObjectFlags() const override
    {
        if (auto flags = interface_cast<IObjectFlags>(object_)) {
            return flags->GetObjectFlags();
        }
        return {};
    }
    void SetObjectFlags(const ObjectFlagBitsValue& value) override
    {
        if (auto flags = interface_cast<IObjectFlags>(object_)) {
            flags->SetObjectFlags(value);
        }
    }
    ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        if (auto flags = interface_cast<IObjectFlags>(object_)) {
            return flags->GetObjectDefaultFlags();
        }
        return {};
    }

protected: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override
    {
#ifdef _DEBUG
        // Object registry calls this
        if (buildCalled_) {
            CORE_LOG_E("Do not call Build explicitly from derived class!");
        }
        buildCalled_ = true;
#endif
        return true;
    }

    void SetInstanceId(InstanceId uid) override
    {
        // Object registry does this
    }
    void Destroy() override
    {
        if (auto builder = interface_cast<META_NS::ILifecycle>(object_)) {
            builder->Destroy();
        }
    }

protected: // IDerived
    void SetSuperInstance(const META_NS::IObject::Ptr& /*aggr*/, const META_NS::IObject::Ptr& super) override
    {
#ifdef _DEBUG
        // Object registry calls this
        if (object_) {
            CORE_LOG_E("Do not call SetSuperInstance explicitly from derived class!");
        }
#endif
        if (IsFirstInit()) {
            if (auto i = interface_cast<IMetadataInternal>(super)) {
                StaticObjectMeta().baseclass = &i->GetStaticMetadata();
            }
        }

        object_ = interface_pointer_cast<IObjectInstance>(super); // Save the strong reference to super.
    }
    BASE_NS::Uid GetSuperClassUid() const override
    {
        return SuperClassInfo.Id().ToUid();
    }

protected:
    bool IsFirstInit()
    {
        static bool init = (isFirstInit_ = true);
        return isFirstInit_;
    }

    template<typename Type>
    nullptr_t RegisterStaticPropertyMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name,
        ObjectFlagBitsValue flags, Internal::PCtor* ctor, Internal::PMemberInit* init)
    {
        if (IsFirstInit()) {
            StaticObjectMeta().properties.push_back(
                PropertyMetadata { name, intf, flags, UidFromType<Type>(), ctor, init });
        }
        return nullptr;
    }

    template<typename Type>
    nullptr_t RegisterStaticEventMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name,
        Internal::ECtor* ctor, Internal::EMemberInit* init)
    {
        if (IsFirstInit()) {
            StaticObjectMeta().events.push_back(EventMetadata { name, intf, Type::UID, ctor, init });
        }
        return nullptr;
    }

    nullptr_t RegisterStaticFunctionMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name,
        Internal::FCtor* ctor, Internal::FContext* context)
    {
        if (IsFirstInit()) {
            StaticObjectMeta().functions.push_back(FunctionMetadata { name, intf, ctor, context });
        }
        return nullptr;
    }

protected:
    static StaticObjectMetadata& StaticObjectMeta()
    {
        static StaticObjectMetadata meta { ClassInfo, nullptr };
        return meta;
    }

    IObject::Ptr GetBase() const noexcept
    {
        return object_;
    }

    template<typename Type>
    constexpr Type* GetBaseAs()
    {
        auto* t = object_->GetInterface<Type>();
        CORE_ASSERT_MSG(t, "Invalid interface %s for base", BASE_NS::to_string(Type::UID).c_str());
        return t;
    }
    template<typename Type>
    constexpr const Type* GetBaseAs() const
    {
        return const_cast<BaseObjectFwd*>(this)->GetBaseAs<Type>();
    }

protected:
    BaseObjectFwd() = default;
    ~BaseObjectFwd() override = default;
    META_NO_COPY_MOVE(BaseObjectFwd)

private:
    bool isFirstInit_ {};
    IObjectInstance::Ptr object_;
#ifdef _DEBUG
    bool buildCalled_ {};
#endif
};

/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Makes a forwarder for an interface
 *        property implemented by the super class.
 */
#define META_EXT_BASE_READONLY_PROPERTY(Interface, Type, Name) \
    META_FORWARD_READONLY_PROPERTY(Type, Name, Super::template GetBaseAs<Interface>()->Name())

/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Makes a forwarder for an interface
 *        property defined by the super class.
 */
#define META_EXT_BASE_PROPERTY(Interface, Type, Name) \
    META_FORWARD_PROPERTY(Type, Name, Super::template GetBaseAs<Interface>()->Name())

/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Calls a method through an interface defined
 *        by the super class.
 */
#define META_EXT_CALL_BASE(Interface, Function) Super::template GetBaseAs<Interface>()->Function

/**
 * @brief A helper template class for implementing a class which implements a basic set of object interfaces with
 *        the addition of Metadata related interfaces.
 * @note Usually when inheriting from this template directly SuperClassInfo should be META_NS::ClassId::MetaObject.
 */
template<class FinalClass, const META_NS::ClassInfo& ClassInfo, const META_NS::ClassInfo& SuperClassInfo,
    class... Interfaces>
class MetaObjectFwd : public BaseObjectFwd<FinalClass, ClassInfo, SuperClassInfo, IMetadata, IMetadataInternal,
                          IObjectContextProvider, Interfaces...> {
    using Super = BaseObjectFwd<FinalClass, ClassInfo, SuperClassInfo, IMetadata, IMetadataInternal,
        IObjectContextProvider, Interfaces...>;

protected: // IMetadata
    IContainer::Ptr GetPropertyContainer() override
    {
        return meta_->GetPropertyContainer();
    }

    IContainer::ConstPtr GetPropertyContainer() const override
    {
        return meta_->GetPropertyContainer();
    }

protected:
    IMetadata::Ptr CloneMetadata() const override
    {
        return meta_->CloneMetadata();
    }
    void AddProperty(const IProperty::Ptr& p) override
    {
        auto self = Super::GetSelf();
        if (self) {
            if (auto pp = interface_pointer_cast<IPropertyInternal>(p)) {
                pp->SetOwner(self);
            }
        }
        meta_->AddProperty(p);
    }
    void RemoveProperty(const IProperty::Ptr& p) override
    {
        meta_->RemoveProperty(p);
    }
    void AddFunction(const IFunction::Ptr& p) override
    {
        meta_->AddFunction(p);
    }
    void RemoveFunction(const IFunction::Ptr& p) override
    {
        meta_->RemoveFunction(p);
    }
    void AddEvent(const IEvent::Ptr& p) override
    {
        meta_->AddEvent(p);
    }
    void RemoveEvent(const IEvent::Ptr& p) override
    {
        meta_->RemoveEvent(p);
    }
    void SetProperties(const BASE_NS::vector<IProperty::Ptr>& vec) override
    {
        meta_->SetProperties(vec);
    }
    void Merge(const IMetadata::Ptr& data) override
    {
        meta_->Merge(data);
    }
    BASE_NS::vector<META_NS::IProperty::Ptr> GetAllProperties() override
    {
        return meta_->GetAllProperties();
    }
    BASE_NS::vector<META_NS::IProperty::ConstPtr> GetAllProperties() const override
    {
        return static_cast<const IMetadata*>(meta_.get())->GetAllProperties();
    }
    BASE_NS::vector<IFunction::Ptr> GetAllFunctions() override
    {
        return meta_->GetAllFunctions();
    }
    BASE_NS::vector<IFunction::ConstPtr> GetAllFunctions() const override
    {
        return static_cast<const IMetadata*>(meta_.get())->GetAllFunctions();
    }
    BASE_NS::vector<IEvent::Ptr> GetAllEvents() override
    {
        return meta_->GetAllEvents();
    }
    BASE_NS::vector<IEvent::ConstPtr> GetAllEvents() const override
    {
        return static_cast<const IMetadata*>(meta_.get())->GetAllEvents();
    }

    IProperty::Ptr GetPropertyByName(BASE_NS::string_view name) override
    {
        return meta_->GetPropertyByName(name);
    }
    IProperty::ConstPtr GetPropertyByName(BASE_NS::string_view name) const override
    {
        return meta_->GetPropertyByName(name);
    }

    IFunction::ConstPtr GetFunctionByName(BASE_NS::string_view name) const override
    {
        return meta_->GetFunctionByName(name);
    }
    IFunction::Ptr GetFunctionByName(BASE_NS::string_view name) override
    {
        return meta_->GetFunctionByName(name);
    }
    IEvent::ConstPtr GetEventByName(BASE_NS::string_view name) const override
    {
        return meta_->GetEventByName(name);
    }
    IEvent::Ptr GetEventByName(BASE_NS::string_view name) override
    {
        return meta_->GetEventByName(name);
    }

protected: // IMetadataInternal
    void SetMetadata(const IMetadata::Ptr& meta) override
    {
        meta_ = meta;
        ConstructPropertiesFromMetadata(this, FinalClass::StaticObjectMeta(), meta);
        ConstructEventsFromMetadata(this, FinalClass::StaticObjectMeta(), meta);
        ConstructFunctionsFromMetadata(this, FinalClass::StaticObjectMeta(), meta);
    }

    IMetadata::Ptr GetMetadata() const override
    {
        return meta_;
    }

    const StaticObjectMetadata& GetStaticMetadata() const override
    {
        return FinalClass::GetStaticObjectMetadata();
    }

protected: // IObjectContextProvider
    META_EXT_BASE_PROPERTY(IObjectContextProvider, IObjectContext::Ptr, ObjectContext)
    void ResetObjectContext() override
    {
        META_EXT_CALL_BASE(IObjectContextProvider, ResetObjectContext());
    }

protected:
    MetaObjectFwd() = default;
    ~MetaObjectFwd() override = default;
    META_NO_COPY_MOVE(MetaObjectFwd)

private:
    IMetadata::Ptr meta_;
};

/**
 * @brief A helper template class for implementing a class which implements the full set of object interfaces.
 * @note Usually this template should be used as the base class, unless SuperClassInfo is
 *       META_NS::ClassId::BaseObject or META_NS::ClassId::MetaObject.
 */
template<class FinalClass, const META_NS::ClassInfo& ClassInfo, const META_NS::ClassInfo& SuperClassInfo,
    class... Interfaces>
class ObjectFwd : public MetaObjectFwd<FinalClass, ClassInfo, SuperClassInfo, IAttach, IContainerQuery, Interfaces...> {
    using Super = MetaObjectFwd<FinalClass, ClassInfo, SuperClassInfo, IAttach, IContainerQuery, Interfaces...>;

public:
    META_NS::IObjectRegistry& GetObjectRegistry() override
    {
        if (auto context = Super::ObjectContext()->GetValue()) {
            return context->GetObjectRegistry();
        }
        return Super::GetObjectRegistry();
    }

protected: // IAttach
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override
    {
        return META_EXT_CALL_BASE(IAttach, Attach(attachment, dataContext));
    }

    bool Detach(const IObject::Ptr& attachment) override
    {
        return META_EXT_CALL_BASE(IAttach, Detach(attachment));
    }
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const override
    {
        return META_EXT_CALL_BASE(IAttach, GetAttachments(uids, strict));
    }
    bool HasAttachments() const override
    {
        return META_EXT_CALL_BASE(IAttach, HasAttachments());
    }
    IContainer::Ptr GetAttachmentContainer(bool initializeAlways) const override
    {
        return META_EXT_CALL_BASE(IAttach, GetAttachmentContainer(initializeAlways));
    }

protected: // IContainerQuery
    BASE_NS::vector<IContainer::Ptr> FindAllContainers(
        const IContainerQuery::ContainerFindOptions& options) const override
    {
        return META_EXT_CALL_BASE(IContainerQuery, FindAllContainers(options));
    }

protected:
    ObjectFwd() = default;
    ~ObjectFwd() override = default;
    META_NO_COPY_MOVE(ObjectFwd)
};

META_END_NAMESPACE()

#endif
