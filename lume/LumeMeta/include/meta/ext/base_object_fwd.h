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

#ifndef META_EXT_BASE_OBJECT_FWD_H
#define META_EXT_BASE_OBJECT_FWD_H

#include <base/util/uid_util.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>

#include "object_factory.h"

META_BEGIN_NAMESPACE()
/**
 * @brief A helper class for implementing a class using "aggregate inheritance" which implements a basic set of object
 * interfaces.
 */
class BaseObjectFwd : public IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle, IStaticMetadata> {
    using Super = IntroduceInterfaces<IObjectInstance, IObjectFlags, IDerived, ILifecycle, IStaticMetadata>;

public:
    inline static const nullptr_t STATIC_METADATA {};
    static const StaticObjectMetadata* StaticMetadata()
    {
        static const META_NS::StaticObjectMetadata objdata { nullptr, nullptr,
            META_NS::GetAggregateMetadata(ClassId::BaseObject), nullptr, 0 };
        return &objdata;
    }
    const StaticObjectMetadata* GetStaticMetadata() const override
    {
        return StaticMetadata();
    }

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

public: // IObject
    InstanceId GetInstanceId() const override
    {
        return object_->GetInstanceId();
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
        object_ = interface_pointer_cast<IObjectInstance>(super); // Save the strong reference to super.
        assert(object_);
    }

protected:
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
    IObjectInstance::Ptr object_;
#ifdef _DEBUG
    bool buildCalled_ {};
#endif
};

META_END_NAMESPACE()

#endif
