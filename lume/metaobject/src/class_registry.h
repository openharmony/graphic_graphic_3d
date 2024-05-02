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
#ifndef META_SRC_CLASS_REGISTRY_H
#define META_SRC_CLASS_REGISTRY_H

#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/ext/event_impl.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_object_factory.h>
#include <meta/interface/object_type_info.h>

META_BEGIN_NAMESPACE()

/** If strict=true, return true if all bits from bitmask are set in compareTo.
 *  If strict=false, return true if any bits from bitmask are set in compareTo. */
constexpr inline bool CheckCategoryBits(ObjectCategoryBits compareTo, ObjectCategoryBits bitmask, bool strict)
{
    return strict ? (compareTo & bitmask) == bitmask : (compareTo & bitmask) != 0;
}

class ClassRegistry final : public IntroduceInterfaces<IClassRegistry> {
public:
    void Clear();
    bool Register(const IObjectFactory::Ptr& fac);
    bool Unregister(const IObjectFactory::Ptr& fac);

    BASE_NS::string GetClassName(BASE_NS::Uid uid) const;
    IObjectFactory::ConstPtr GetObjectFactory(const BASE_NS::Uid& uid) const;
    BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(
        ObjectCategoryBits category, bool strict, bool excludeDeprecated) const;

    BASE_NS::shared_ptr<IEvent> EventOnClassRegistered() const override
    {
        return onRegistered_;
    }
    BASE_NS::shared_ptr<IEvent> EventOnClassUnregistered() const override
    {
        return onUnregistered_;
    }

public: // IClassRegistry
    BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(
        const BASE_NS::vector<BASE_NS::Uid>& interfaceUids, bool strict, bool excludeDeprecated) const override;

private:
    mutable std::shared_mutex mutex_;
    mutable BASE_NS::unordered_map<ObjectId, IObjectFactory::Ptr> objectFactories_;

    mutable BASE_NS::shared_ptr<EventImpl<IOnClassRegistrationChanged>> onRegistered_ {
        CreateShared<EventImpl<IOnClassRegistrationChanged>>("OnClassRegistered")
    };
    mutable BASE_NS::shared_ptr<EventImpl<IOnClassRegistrationChanged>> onUnregistered_ {
        CreateShared<EventImpl<IOnClassRegistrationChanged>>("OnClassRegistered")
    };
};

META_END_NAMESPACE()

#endif // META_SRC_CLASS_REGISTRY_H
