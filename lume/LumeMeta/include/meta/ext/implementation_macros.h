/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: implementation macros
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-13
 */

#ifndef META_EXT_IMPLEMENTATION_MACROS_H
#define META_EXT_IMPLEMENTATION_MACROS_H

#include <base/containers/shared_ptr.h>

#include <meta/base/namespace.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_owner.h>
#include <meta/interface/metadata_query.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

/**
 * @brief Access property variable introduced with META_IMPLEMENT_*_PROPERTY
 */
#define META_ACCESS_PROPERTY(name) name()
#define META_ACCESS_PROPERTY_VALUE(name) name()->GetValue()
/**
 * @brief Access event variable introduced with META_IMPLEMENT_*_EVENT
 */
#define META_ACCESS_EVENT(name) name()

#define META_VALUE_PTR(interface, classId) ::META_NS::ValuePtrImpl<interface>::Instance<classId>

META_BEGIN_NAMESPACE()
constexpr ObjectFlagBitsValue DEFAULT_PROPERTY_FLAGS = ObjectFlagBitsValue { ObjectFlagBits::SERIALIZE };
constexpr ObjectFlagBitsValue DEFAULT_PROPERTY_FLAGS_NO_SER = ObjectFlagBitsValue { ObjectFlagBits::NONE };

template<typename Type>
IProperty::Ptr CreatePropertyImpl(BASE_NS::string_view name, Internal::MetaValue* def, ObjectFlagBitsValue flags)
{
    flags |= ObjectFlagBits::NATIVE;
    IAny::Ptr value;
    if (def) {
        value = def();
    }
    if constexpr (BASE_NS::is_array_v<Type>) {
        if (value) {
            return META_NS::ConstructArrayPropertyAny<BASE_NS::remove_extent_t<Type>>(name, *value, flags);
        }
        return META_NS::ConstructArrayProperty<BASE_NS::remove_extent_t<Type>>(name, {}, flags);
    } else {
        if (value) {
            return META_NS::ConstructPropertyAny<Type>(name, *value, flags);
        }
        return META_NS::ConstructProperty<Type>(name, {}, flags);
    }
}

template<typename Type, uint64_t Flags>
Internal::MetadataCtor* CreatePropertyConstructor()
{
    return [](const BASE_NS::shared_ptr<IOwner>& owner, const StaticMetadata& d) {
        auto ret = interface_pointer_cast<CORE_NS::IInterface>(
            META_NS::CreatePropertyImpl<Type>(d.name, d.runtimeValue, ObjectFlagBitsValue(Flags)));
        if (ret) {
            if (auto ctor = interface_cast<IMetadataOwner>(owner)) {
                ctor->OnMetadataConstructed(d, *ret);
            }
        }
        return ret;
    };
}

template<typename Type>
Internal::MetadataCtor* CreateEventConstructor()
{
    return [](const BASE_NS::shared_ptr<IOwner>& owner, const StaticMetadata& d) {
        auto ret = interface_pointer_cast<CORE_NS::IInterface>(CreateShared<EventImpl<Type>>(d.name));
        if (ret) {
            if (auto ctor = interface_cast<IMetadataOwner>(owner)) {
                ctor->OnMetadataConstructed(d, *ret);
            }
        }
        return ret;
    };
}

template<typename Interface, auto MemFun>
Internal::MetadataCtor* CreateFunctionConstructor()
{
    return [](const BASE_NS::shared_ptr<IOwner>& owner, const StaticMetadata& d) {
        auto ret = interface_pointer_cast<CORE_NS::IInterface>(
            CreateFunction(d.name, interface_pointer_cast<Interface>(owner), MemFun, d.runtimeValue));
        if (ret) {
            if (auto ctor = interface_cast<IMetadataOwner>(owner)) {
                ctor->OnMetadataConstructed(d, *ret);
            }
        }
        return ret;
    };
}

template<size_t Size>
constexpr size_t MetadataArraySize(const StaticMetadata (&)[Size])
{
    return Size;
}
constexpr size_t MetadataArraySize(const nullptr_t&)
{
    return 0;
}

inline const StaticObjectMetadata* GetAggregateMetadata(const ObjectId& id)
{
    auto f = GetObjectRegistry().GetObjectFactory(id);
    return f ? f->GetClassStaticMetadata() : nullptr;
}

template<typename Type, typename Base, typename Data>
inline StaticObjectMetadata CreateStaticMetadata(const Data& data, const StaticObjectMetadata* base,
    const ClassInfo* classInfo, const StaticObjectMetadata* aggregate)
{
    size_t size = META_NS::MetadataArraySize(data);
    bool baseDataSame = size && base && data == base->metadata;
    return StaticObjectMetadata { classInfo, base, aggregate, baseDataSame ? nullptr : data, baseDataSame ? 0 : size };
}

template<typename T>
inline uint8_t GetPropertySMDFlags(BASE_NS::shared_ptr<META_NS::IProperty> (T::*)(), int)
{
    return 0;
}

template<typename T>
inline uint8_t GetPropertySMDFlags(BASE_NS::shared_ptr<const META_NS::IProperty> (T::*)() const, short)
{
    return static_cast<uint8_t>(Internal::PropertyFlag::READONLY);
}

META_END_NAMESPACE()

#define META_IMPLEMENT_STATIC_DATA_DEPIMPL(myClass, classInfo, baseClass)                                       \
private:                                                                                                        \
    using Super = baseClass;                                                                                    \
                                                                                                                \
public:                                                                                                         \
    static const META_NS::StaticObjectMetadata* StaticMetadata()                                                \
    {                                                                                                           \
        static const META_NS::StaticObjectMetadata objdata { META_NS::CreateStaticMetadata<myClass, baseClass>( \
            myClass::STATIC_METADATA, baseClass::StaticMetadata(), classInfo, nullptr) };                       \
        return &objdata;                                                                                        \
    }                                                                                                           \
    const META_NS::StaticObjectMetadata* GetStaticMetadata() const override                                     \
    {                                                                                                           \
        return StaticMetadata();                                                                                \
    }

#define META_IMPLEMENT_STATIC_DATA_AGGRIMPL(myClass, classInfo, baseClass, aggrClass)                           \
private:                                                                                                        \
    using Super = baseClass;                                                                                    \
                                                                                                                \
public:                                                                                                         \
    static const META_NS::StaticObjectMetadata* StaticMetadata()                                                \
    {                                                                                                           \
        static const META_NS::StaticObjectMetadata objdata { META_NS::CreateStaticMetadata<myClass, baseClass>( \
            myClass::STATIC_METADATA, baseClass::StaticMetadata(), classInfo,                                   \
            META_NS::GetAggregateMetadata(aggrClass)) };                                                        \
        return &objdata;                                                                                        \
    }                                                                                                           \
    const META_NS::StaticObjectMetadata* GetStaticMetadata() const override                                     \
    {                                                                                                           \
        return StaticMetadata();                                                                                \
    }

#define META_BEGIN_STATIC_DATA() inline static const META_NS::StaticMetadata STATIC_METADATA[] = {
#define META_END_STATIC_DATA() \
    }                          \
    ;

#define META_IMPLEMENT_OBJECT_TYPE_INTERFACE(classInfo)          \
    META_NS::ObjectId GetClassId() const override                \
    {                                                            \
        return classInfo.Id();                                   \
    }                                                            \
    BASE_NS::string_view GetClassName() const override           \
    {                                                            \
        return classInfo.Name();                                 \
    }                                                            \
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override \
    {                                                            \
        return GetInterfacesVector();                            \
    }

#define META_IMPLEMENT_OBJECT_BASE_AGGR(myClass, classInfo, baseClass, aggrClass)  \
    META_IMPLEMENT_STATIC_DATA_AGGRIMPL(myClass, &classInfo, baseClass, aggrClass) \
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(classInfo)                                \
    META_DEFINE_OBJECT_TYPE_INFO(myClass, classInfo)

#define META_IMPLEMENT_OBJECT_BASE(myClass, classInfo, baseClass)      \
    META_IMPLEMENT_STATIC_DATA_DEPIMPL(myClass, &classInfo, baseClass) \
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(classInfo)                    \
    META_DEFINE_OBJECT_TYPE_INFO(myClass, classInfo)

#define META_OBJECT(...) \
    META_EXPAND(         \
        META_GET_MACRO4_IMPL(__VA_ARGS__, META_IMPLEMENT_OBJECT_BASE_AGGR, META_IMPLEMENT_OBJECT_BASE)(__VA_ARGS__))

#define META_IMPLEMENT_OBJECT_BASE_AGGR_NO_CLASSINFO(myClass, baseClass, aggrClass) \
    META_IMPLEMENT_STATIC_DATA_AGGRIMPL(myClass, nullptr, baseClass, aggrClass)

#define META_IMPLEMENT_OBJECT_BASE_NO_CLASSINFO(myClass, baseClass) \
    META_IMPLEMENT_STATIC_DATA_DEPIMPL(myClass, nullptr, baseClass)

#define META_OBJECT_NO_CLASSINFO(...)                                                           \
    META_EXPAND(META_GET_MACRO3_IMPL(__VA_ARGS__, META_IMPLEMENT_OBJECT_BASE_AGGR_NO_CLASSINFO, \
        META_IMPLEMENT_OBJECT_BASE_NO_CLASSINFO)(__VA_ARGS__))

#if 0
#define META_INTF_CHECK(interface, type, name)                                                \
    [](const auto* intf) {                                                                    \
        if constexpr (!BASE_NS::is_same_v<decltype(intf), const META_NS::NoInterface*>) {     \
            using MyMetaValueType = typename decltype(intf->name())::ValueType;               \
            static_assert(BASE_NS::is_same_v<META_NS::PropertyType_v<type>, MyMetaValueType>, \
                "Invalid property data, type not matching");                                  \
        }                                                                                     \
        return interface::INTERFACE_INFO;                                                     \
    }((interface*)nullptr)
#else
#define META_INTF_CHECK(interface, type, name) interface::INTERFACE_INFO
#endif

// todo: check in META_STATIC_PROPERTY_DATA and META_STATIC_EVENT_DATA that name is part of the given interface
//--- PROPERTY DATA
#define META_IMPL_PROPERTY_DATA_VALUE_FLAGS_SMDF(intfuid, type, name, value, flags, smdflags)       \
    { META_NS::MetadataType::PROPERTY, intfuid, #name,                                              \
        META_NS::CreatePropertyConstructor<type, META_NS::ObjectFlagBitsValue(flags).GetValue()>(), \
        [] { return META_NS::ConstructAnyHelper<type>(value); }, nullptr, smdflags },

#define META_IMPL_PROPERTY_DATA_VALUE_FLAGS(intf, type, name, value, flags)                               \
    META_IMPL_PROPERTY_DATA_VALUE_FLAGS_SMDF(META_INTF_CHECK(intf, type, name), type, name, value, flags, \
        META_NS::GetPropertySMDFlags(&intf::Property##name, 0))
#define META_IMPL_PROPERTY_DATA_VALUE(intf, type, name, value) \
    META_IMPL_PROPERTY_DATA_VALUE_FLAGS(intf, type, name, value, META_NS::DEFAULT_PROPERTY_FLAGS)
#define META_IMPL_PROPERTY_DATA(intf, type, name) META_IMPL_PROPERTY_DATA_VALUE(intf, type, name, {})

#define META_STATIC_PROPERTY_DATA(...)                                                                                \
    META_EXPAND(META_GET_MACRO5_IMPL(__VA_ARGS__, META_IMPL_PROPERTY_DATA_VALUE_FLAGS, META_IMPL_PROPERTY_DATA_VALUE, \
        META_IMPL_PROPERTY_DATA)(__VA_ARGS__))
//---

//--- ARRAY PROPERTY DATA
#define META_IMPL_ARRAY_PROPERTY_DATA_VALUE_FLAGS(intf, type, name, value, flags)                     \
    { META_NS::MetadataType::PROPERTY, META_INTF_CHECK(intf, type, name), #name,                      \
        META_NS::CreatePropertyConstructor<type[], META_NS::ObjectFlagBitsValue(flags).GetValue()>(), \
        [] { return META_NS::IAny::Ptr(META_NS::ConstructArrayAnyHelper<type>(value)); } },

#define META_IMPL_ARRAY_PROPERTY_DATA_VALUE(intf, type, name, value) \
    META_IMPL_ARRAY_PROPERTY_DATA_VALUE_FLAGS(intf, type, name, value, META_NS::DEFAULT_PROPERTY_FLAGS)
#define META_IMPL_ARRAY_PROPERTY_DATA(intf, type, name) META_IMPL_ARRAY_PROPERTY_DATA_VALUE(intf, type, name, {})

#define META_STATIC_ARRAY_PROPERTY_DATA(...)                                                 \
    META_EXPAND(META_GET_MACRO5_IMPL(__VA_ARGS__, META_IMPL_ARRAY_PROPERTY_DATA_VALUE_FLAGS, \
        META_IMPL_ARRAY_PROPERTY_DATA_VALUE, META_IMPL_ARRAY_PROPERTY_DATA)(__VA_ARGS__))
//---

//--- Property forwarding
#define META_STATIC_FORWARDED_PROPERTY_DATA(intf, type, name)                                                  \
    { META_NS::MetadataType::PROPERTY, intf::INTERFACE_INFO, #name,                                            \
        [](const BASE_NS::shared_ptr<META_NS::IOwner>& owner, const META_NS::StaticMetadata&) {                \
            auto i = interface_cast<intf>(owner);                                                              \
            auto p = interface_pointer_cast<::CORE_NS::IInterface>(i ? i->Property##name() : nullptr);         \
            return BASE_NS::shared_ptr<::CORE_NS::IInterface>(p, const_cast<::CORE_NS::IInterface*>(p.get())); \
        },                                                                                                     \
        [] { return META_NS::ConstructAnyHelper<type>({}); }, nullptr,                                         \
        uint8_t(uint8_t(META_NS::Internal::StaticMetaFlag::FORWARD) |                                          \
                META_NS::GetPropertySMDFlags(&intf::Property##name, 0)) },

#define META_STATIC_FORWARDED_ARRAY_PROPERTY_DATA(intf, type, name)                                            \
    { META_NS::MetadataType::PROPERTY, intf::INTERFACE_INFO, #name,                                            \
        [](const BASE_NS::shared_ptr<META_NS::IOwner>& owner, const META_NS::StaticMetadata&) {                \
            auto i = interface_cast<intf>(owner);                                                              \
            auto p = interface_pointer_cast<::CORE_NS::IInterface>(i ? i->Property##name() : nullptr);         \
            return BASE_NS::shared_ptr<::CORE_NS::IInterface>(p, const_cast<::CORE_NS::IInterface*>(p.get())); \
        },                                                                                                     \
        [] { return META_NS::ConstructArrayAnyHelper<type>({}); }, nullptr,                                    \
        uint8_t(uint8_t(META_NS::Internal::StaticMetaFlag::FORWARD) |                                          \
                META_NS::GetPropertySMDFlags(&intf::Property##name, 0)) },
//---

#define META_STATIC_EVENT_DATA(intf, type, name) \
    { META_NS::MetadataType::EVENT, intf::INTERFACE_INFO, #name, META_NS::CreateEventConstructor<type>(), nullptr },

//--- Event forwarding
#define META_STATIC_FORWARDED_EVENT_DATA(intf, type, name)                                      \
    { META_NS::MetadataType::EVENT, intf::INTERFACE_INFO, #name,                                \
        [](const BASE_NS::shared_ptr<META_NS::IOwner>& owner, const META_NS::StaticMetadata&) { \
            auto i = interface_cast<intf>(owner);                                               \
            return interface_pointer_cast<::CORE_NS::IInterface>(                               \
                i ? i->Event##name(META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) : nullptr);    \
        },                                                                                      \
        nullptr, nullptr, uint8_t(META_NS::Internal::StaticMetaFlag::FORWARD) },
//---

#define META_STATIC_FUNCTION_DATA(intf, func, ...)                                          \
    { META_NS::MetadataType::FUNCTION, intf::INTERFACE_INFO, #func,                         \
        META_NS::CreateFunctionConstructor<intf, &intf ::func>(), [] {                      \
            ::BASE_NS::string_view arr[] = { "", __VA_ARGS__ };                             \
            return META_NS::ConstructAny<META_NS::ICallContext::Ptr>(                       \
                META_NS::CreateCallContext(&intf ::func, ::META_NS::ParamNameToView(arr))); \
        } },

#define META_PRIVATE_PROPERTY_TYPED_IMPL(type, name)                                                               \
    META_NS::Property<type> name() noexcept                                                                        \
    {                                                                                                              \
        return META_NS::Property<type> { this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) }; \
    }                                                                                                              \
    META_NS::ConstProperty<type> name() const noexcept                                                             \
    {                                                                                                              \
        return META_NS::ConstProperty<type> { this->GetProperty(                                                   \
            #name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) };                                                \
    }

#define META_PRIVATE_ARRAY_PROPERTY_TYPED_IMPL(type, name)            \
    META_NS::ConstArrayProperty<type> name() const noexcept           \
    {                                                                 \
        return META_NS::ConstArrayProperty<type> { this->GetProperty( \
            #name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) };   \
    }                                                                 \
    META_NS::ArrayProperty<type> name() noexcept                      \
    {                                                                 \
        return META_NS::ArrayProperty<type> { this->GetProperty(      \
            #name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) };   \
    }

#define META_IMPLEMENT_READONLY_PROPERTY(type, name)                        \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                       \
        return this->GetProperty(#name);                                    \
    }                                                                       \
    META_PRIVATE_PROPERTY_TYPED_IMPL(::META_NS::PropertyType_v<type>, name)

#define META_IMPLEMENT_CACHED_READONLY_PROPERTY(type, name)                                                 \
    mutable META_NS::Property<::META_NS::PropertyType_v<type>> metaProperty##name##_;                       \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override                                 \
    {                                                                                                       \
        if (!metaProperty##name##_) {                                                                       \
            metaProperty##name##_ = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
        }                                                                                                   \
        return metaProperty##name##_;                                                                       \
    }                                                                                                       \
    META_PRIVATE_PROPERTY_TYPED_IMPL(::META_NS::PropertyType_v<type>, name)

#define META_IMPLEMENT_PROPERTY(type, name)                                                                   \
    META_IMPLEMENT_READONLY_PROPERTY(type, name)                                                              \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                                              \
    {                                                                                                         \
        ::META_NS::IProperty::Ptr p = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
        CORE_ASSERT_MSG(p, "Failed to get property '" #name "'");                                             \
        return p;                                                                                             \
    }

#define META_IMPLEMENT_CACHED_PROPERTY(type, name)                                                          \
    META_IMPLEMENT_CACHED_READONLY_PROPERTY(type, name)                                                     \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                                            \
    {                                                                                                       \
        if (!metaProperty##name##_) {                                                                       \
            metaProperty##name##_ = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
            CORE_ASSERT_MSG(metaProperty##name##_, "Failed to get property '" #name "'");                   \
        }                                                                                                   \
        return metaProperty##name##_;                                                                       \
    }

#define META_IMPLEMENT_READONLY_ARRAY_PROPERTY(type, name)                                                         \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override                                        \
    {                                                                                                              \
        ::META_NS::IProperty::ConstPtr p = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
        CORE_ASSERT_MSG(p, "Failed to get property '" #name "'");                                                  \
        return p;                                                                                                  \
    }                                                                                                              \
    META_PRIVATE_ARRAY_PROPERTY_TYPED_IMPL(type, name)

#define META_IMPLEMENT_CACHED_READONLY_ARRAY_PROPERTY(type, name)                                           \
    mutable META_NS::ArrayProperty<::META_NS::PropertyType_v<type>> metaProperty##name##_;                  \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override                                 \
    {                                                                                                       \
        if (!metaProperty##name##_) {                                                                       \
            metaProperty##name##_ = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
            CORE_ASSERT_MSG(metaProperty##name##_, "Failed to get property '" #name "'");                   \
        }                                                                                                   \
        return metaProperty##name##_;                                                                       \
    }                                                                                                       \
    META_PRIVATE_ARRAY_PROPERTY_TYPED_IMPL(type, name)

#define META_IMPLEMENT_ARRAY_PROPERTY(type, name)                                                             \
    META_IMPLEMENT_READONLY_ARRAY_PROPERTY(type, name)                                                        \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                                              \
    {                                                                                                         \
        ::META_NS::IProperty::Ptr p = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
        CORE_ASSERT_MSG(p, "Failed to get property '" #name "'");                                             \
        return p;                                                                                             \
    }

#define META_IMPLEMENT_CACHED_ARRAY_PROPERTY(type, name)                                                    \
    META_IMPLEMENT_CACHED_READONLY_PROPERTY(type, name)                                                     \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                                            \
    {                                                                                                       \
        if (!metaProperty##name##_) {                                                                       \
            metaProperty##name##_ = this->GetProperty(#name, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
            CORE_ASSERT_MSG(metaProperty##name##_, "Failed to get property '" #name "'");                   \
        }                                                                                                   \
        return metaProperty##name##_;                                                                       \
    }

/**
 * @brief Make forwarding function for the read-only property.
 */
#define META_FORWARD_READONLY_PROPERTY(type, name, forwarder)             \
    META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                     \
        return forwarder;                                                 \
    }                                                                     \
    META_READONLY_PROPERTY_TYPED_IMPL(type, name)

/**
 * @brief Make forwarding function for the property.
 */
#define META_FORWARD_PROPERTY(type, name, forwarder)           \
    META_FORWARD_READONLY_PROPERTY(type, name, forwarder)      \
    META_NS::IProperty::Ptr Property##name() noexcept override \
    {                                                          \
        return forwarder;                                      \
    }                                                          \
    META_PROPERTY_TYPED_IMPL(type, name)

#define META_FORWARD_BASE_READONLY_PROPERTY(type, name) \
    META_FORWARD_READONLY_PROPERTY(type, name, this->Super::Property##name())

#define META_FORWARD_BASE_PROPERTY(type, name) META_FORWARD_PROPERTY(type, name, this->Super::Property##name())

#define META_FORWARD_READONLY_ARRAY_PROPERTY(type, name, forwarder)       \
    META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                     \
        return forwarder;                                                 \
    }                                                                     \
    META_READONLY_ARRAY_PROPERTY_TYPED_IMPL(type, name)

/**
 * @brief Make forwarding function for the property.
 */
#define META_FORWARD_ARRAY_PROPERTY(type, name, forwarder)      \
    META_FORWARD_READONLY_ARRAY_PROPERTY(type, name, forwarder) \
    META_NS::IProperty::Ptr Property##name() noexcept override  \
    {                                                           \
        return forwarder;                                       \
    }                                                           \
    META_ARRAY_PROPERTY_TYPED_IMPL(type, name)

#define META_FORWARD_BASE_READONLY_ARRAY_PROPERTY(type, name) \
    META_FORWARD_READONLY_ARRAY_PROPERTY(type, name, this->Super::Property##name())

#define META_FORWARD_BASE_ARRAY_PROPERTY(type, name) \
    META_FORWARD_ARRAY_PROPERTY(type, name, this->Super::Property##name())

#define META_IMPLEMENT_EVENT(type, name)                                                         \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name(META_NS::MetadataQuery q) const override  \
    {                                                                                            \
        BASE_NS::shared_ptr<const ::META_NS::IEvent> p = this->GetEvent(#name, q);               \
        return BASE_NS::shared_ptr<::META_NS::IEvent>(p, const_cast<META_NS::IEvent*>(p.get())); \
    }                                                                                            \
    META_EVENT_TYPED_IMPL(type, name)

#define META_FORWARD_EVENT(type, name, forwarder)                                               \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name(META_NS::MetadataQuery q) const override \
    {                                                                                           \
        return forwarder(q);                                                                    \
    }                                                                                           \
    META_EVENT_TYPED_IMPL(type, name)

#define META_FORWARD_EVENT_CLASS(type, name, targetClass)                                       \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name(META_NS::MetadataQuery q) const override \
    {                                                                                           \
        return targetClass::Event##name(q);                                                     \
    }                                                                                           \
    ::META_NS::Event<type> name() const                                                         \
    {                                                                                           \
        return targetClass::Event##name(q);                                                     \
    }

#endif
