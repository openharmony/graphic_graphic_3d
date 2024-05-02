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

#ifndef META_EXT_METADATA_HELPERS_H
#define META_EXT_METADATA_HELPERS_H

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property_internal.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Helper to construct/add properties to newly created object using the static metadata.
 */
template<typename Self>
void ConstructPropertiesFromMetadata(Self* self, const StaticObjectMetadata& sm, const IMetadata::Ptr& meta)
{
    for (auto&& pm : sm.properties) {
        auto p = meta->GetPropertyByName(pm.name);
        if (p) {
            if (!pm.init(self, p)) {
                p = nullptr;
            }
        }
        if (!p) {
            p = pm.create();
            if (!pm.init(self, p)) {
                CORE_ASSERT_MSG(false, "Failed to initialise property");
                return;
            }
            meta->AddProperty(p);
        }
        if (auto pp = interface_cast<IPropertyInternal>(p)) {
            if (auto s = self->GetInterface(IObjectInstance::UID)) {
                pp->SetOwner(static_cast<IObjectInstance*>(s)->GetSelf());
            }
        }
    }
}

/**
 * @brief Helper to construct or init event.
 */
template<typename Self>
bool ConstructOrInitEvent(IEvent::Ptr& p, Self* self, const EventMetadata& pm)
{
    if (p) {
        if (!pm.init(self, p)) {
            p = nullptr;
        }
    }
    if (!p) {
        p = pm.create();
        if (!pm.init(self, p)) {
            CORE_ASSERT_MSG(false, "Failed to initialise event");
            return false;
        }
        return true;
    }
    return false;
}

/**
 * @brief Helper to construct/add events to newly created object using the static metadata.
 */
template<typename Self>
void ConstructEventsFromMetadata(Self* self, const StaticObjectMetadata& sm, const IMetadata::Ptr& meta)
{
    for (auto&& pm : sm.events) {
        auto p = meta->GetEventByName(pm.name);
        if (ConstructOrInitEvent(p, self, pm)) {
            meta->AddEvent(p);
        }
    }
}

/**
 * @brief Helper to construct/add meta functions to newly created object using the static metadata.
 */
template<typename Self>
void ConstructFunctionsFromMetadata(Self* self, const StaticObjectMetadata& sm, const IMetadata::Ptr& meta)
{
    for (auto&& pm : sm.functions) {
        auto p = pm.create(self);
        if (!p) {
            CORE_ASSERT_MSG(false, "Failed to create function");
            return;
        }
        meta->AddFunction(p);
    }
}

/**
 * @brief Add static metadata machinery to object. This is usually done by ObjectFwd and a like helpers.
 */
#define STATIC_METADATA_MACHINERY(classinfo, baseclass)                                                              \
    using Impl = baseclass;                                                                                          \
                                                                                                                     \
protected:                                                                                                           \
    bool IsFirstInit()                                                                                               \
    {                                                                                                                \
        static bool init = (isFirstInit_ = true);                                                                    \
        return isFirstInit_;                                                                                         \
    }                                                                                                                \
    template<typename Type>                                                                                          \
    META_NS::nullptr_t RegisterStaticPropertyMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name, \
        META_NS::ObjectFlagBitsValue flags, META_NS::Internal::PCtor* ctor, META_NS::Internal::PMemberInit* init)    \
    {                                                                                                                \
        if (IsFirstInit()) {                                                                                         \
            StaticObjectMeta().properties.push_back(                                                                 \
                META_NS::PropertyMetadata { name, intf, flags, META_NS::UidFromType<Type>(), ctor, init });          \
        }                                                                                                            \
        return nullptr;                                                                                              \
    }                                                                                                                \
    template<typename Type>                                                                                          \
    META_NS::nullptr_t RegisterStaticEventMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name,    \
        META_NS::Internal::ECtor* ctor, META_NS::Internal::EMemberInit* init)                                        \
    {                                                                                                                \
        if (IsFirstInit()) {                                                                                         \
            StaticObjectMeta().events.push_back(META_NS::EventMetadata { name, intf, Type::UID, ctor, init });       \
        }                                                                                                            \
        return nullptr;                                                                                              \
    }                                                                                                                \
    META_NS::nullptr_t RegisterStaticFunctionMetadata(const META_NS::InterfaceInfo& intf, BASE_NS::string_view name, \
        META_NS::Internal::FCtor* ctor, META_NS::Internal::FContext* context)                                        \
    {                                                                                                                \
        if (IsFirstInit()) {                                                                                         \
            StaticObjectMeta().functions.push_back(META_NS::FunctionMetadata { name, intf, ctor, context });         \
        }                                                                                                            \
        return nullptr;                                                                                              \
    }                                                                                                                \
                                                                                                                     \
private:                                                                                                             \
    bool isFirstInit_ {};                                                                                            \
    static META_NS::StaticObjectMetadata& StaticObjectMeta()                                                         \
    {                                                                                                                \
        static META_NS::StaticObjectMetadata meta { classinfo, &Impl::GetStaticObjectMetadata() };                   \
        return meta;                                                                                                 \
    }                                                                                                                \
                                                                                                                     \
public:                                                                                                              \
    static const META_NS::StaticObjectMetadata& GetStaticObjectMetadata()                                            \
    {                                                                                                                \
        return StaticObjectMeta();                                                                                   \
    }                                                                                                                \
                                                                                                                     \
private:

#define STATIC_METADATA_MACHINERY_WITH_CONCRETE_BASE(classinfo, baseclass)        \
    STATIC_METADATA_MACHINERY(classinfo, baseclass)                               \
protected:                                                                        \
    const META_NS::StaticObjectMetadata& GetStaticMetadata() const override       \
    {                                                                             \
        return GetStaticObjectMetadata();                                         \
    }                                                                             \
    void SetMetadata(const META_NS::IMetadata::Ptr& meta) override                \
    {                                                                             \
        Impl::SetMetadata(meta);                                                  \
        META_NS::ConstructPropertiesFromMetadata(this, StaticObjectMeta(), meta); \
        META_NS::ConstructEventsFromMetadata(this, StaticObjectMeta(), meta);     \
        META_NS::ConstructFunctionsFromMetadata(this, StaticObjectMeta(), meta);  \
    }

#define STATIC_METADATA_WITH_CONCRETE_BASE(introduced, baseclass) \
    STATIC_METADATA_MACHINERY_WITH_CONCRETE_BASE(ClassInfo, baseclass)

META_END_NAMESPACE()

#endif
