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

#ifndef META_EXT_IMPLEMENTATION_MACROS_H
#define META_EXT_IMPLEMENTATION_MACROS_H

#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

/**
 * @brief Access property variable introduced with META_IMPLEMENT_*_PROPERTY
 */
#define META_ACCESS_PROPERTY(name) metaProperty##name##_
#define META_ACCESS_PROPERTY_VALUE(name) metaProperty##name##_->GetValue()
/**
 * @brief Access event variable introduced with META_IMPLEMENT_*_EVENT
 */
#define META_ACCESS_EVENT(name) metaEvent##name##_

#define META_VALUE_PTR(interface, classId) ::META_NS::ValuePtrImpl<interface>::Instance<classId>

META_BEGIN_NAMESPACE()
constexpr ObjectFlagBitsValue DEFAULT_PROPERTY_FLAGS = ObjectFlagBitsValue { ObjectFlagBits::SERIALIZE };
constexpr ObjectFlagBitsValue DEFAULT_PROPERTY_FLAGS_NO_SER = ObjectFlagBitsValue { ObjectFlagBits::NONE };
META_END_NAMESPACE()

#define META_DEFINE_PROPERTY_VAR(intf, name, type, defaultValue, flags)                                               \
    ::META_NS::Property<::META_NS::PropertyType_v<type>> metaProperty##name##_ =                                      \
        this->template RegisterStaticPropertyMetadata<::META_NS::PropertyType_v<type>>(                               \
            intf, #name, flags,                                                                                       \
            [] { return ::META_NS::IProperty::Ptr(::META_NS::ConstructProperty<type>(#name, defaultValue, flags)); }, \
            [](auto me) { /*workaround for older msvc not supporting decltype(this) without capture*/                 \
                return [](void* self, const ::META_NS::IProperty::Ptr& p) {                                           \
                    if (p) {                                                                                          \
                        static_cast<decltype(me)>(self)->metaProperty##name##_ = p;                                   \
                    }                                                                                                 \
                    return p != nullptr;                                                                              \
                };                                                                                                    \
            }(this));

#define META_DEFINE_PROPERTY_VARArray(intf, name, type, defaultValue, flags)                                           \
    ::META_NS::ArrayProperty<type> metaProperty##name##_ = this->template RegisterStaticPropertyMetadata<type[]>(      \
        intf, #name, flags,                                                                                            \
        [] { return ::META_NS::IProperty::Ptr(::META_NS::ConstructArrayProperty<type>(#name, defaultValue, flags)); }, \
        [](auto me) { /*workaround for older msvc not supporting decltype(this) without capture*/                      \
            return [](void* self, const ::META_NS::IProperty::Ptr& p) {                                                \
                if (p) {                                                                                               \
                    static_cast<decltype(me)>(self)->metaProperty##name##_ = p;                                        \
                }                                                                                                      \
                return p != nullptr;                                                                                   \
            };                                                                                                         \
        }(this));

#define META_DEFINE_READONLY_PROPERTY(propType, intf, name, type, defaultValue, flags)                                \
    META_DEFINE_PROPERTY_VAR##propType(                                                                               \
        intf, name, type, defaultValue, flags)::META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                                                                 \
        return metaProperty##name##_;                                                                                 \
    }                                                                                                                 \
    META_READONLY_PROPERTY_TYPED_IMPL(::META_NS::PropertyType_v<type>, name)

#define META_DEFINE_PROPERTY(propType, intf, name, type, defaultValue, flags)      \
    META_DEFINE_READONLY_PROPERTY(propType, intf, name, type, defaultValue, flags) \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                   \
    {                                                                              \
        return metaProperty##name##_;                                              \
    }                                                                              \
    META_PROPERTY_TYPED_IMPL(::META_NS::PropertyType_v<type>, name)

#define META_DEFINE_READONLY_ARRAY_PROPERTY(propType, intf, name, type, defaultValue, flags)                          \
    META_DEFINE_PROPERTY_VAR##propType(                                                                               \
        intf, name, type, defaultValue, flags)::META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                                                                 \
        return metaProperty##name##_;                                                                                 \
    }                                                                                                                 \
    META_READONLY_ARRAY_PROPERTY_TYPED_IMPL(type, name)

#define META_DEFINE_ARRAY_PROPERTY(propType, intf, name, type, defaultValue, flags)      \
    META_DEFINE_READONLY_ARRAY_PROPERTY(propType, intf, name, type, defaultValue, flags) \
    ::META_NS::IProperty::Ptr Property##name() noexcept override                         \
    {                                                                                    \
        return metaProperty##name##_;                                                    \
    }                                                                                    \
    META_ARRAY_PROPERTY_TYPED_IMPL(type, name)

#define META_GET_PROPERTY4(macro, propType, intf, type, name, defaultValue, flags) \
    macro(propType, intf, name, type, defaultValue,                                \
        ::META_NS::ObjectFlagBitsValue { flags } | ::META_NS::ObjectFlagBits::NATIVE)
#define META_GET_PROPERTY3(macro, propType, intf, type, name, defaultValue) \
    macro(propType, intf, name, type, defaultValue,                         \
        ::META_NS::DEFAULT_PROPERTY_FLAGS | ::META_NS::ObjectFlagBits::NATIVE)
#define META_GET_PROPERTY2(macro, propType, intf, type, name) \
    macro(propType, intf, name, type, {}, ::META_NS::DEFAULT_PROPERTY_FLAGS | ::META_NS::ObjectFlagBits::NATIVE)
#define META_GET_PROPERTY(macro, propType, ...)                                                                \
    META_EXPAND(META_GET_MACRO5_IMPL(__VA_ARGS__, META_GET_PROPERTY4, META_GET_PROPERTY3, META_GET_PROPERTY2)( \
        macro, propType, __VA_ARGS__))

/**
 * @brief Implement property that was introduced using META_PROPERTY in interface.
 *        This will add the property to static metadata and generates code to construct
 *        it when the object is constructed.
 */
#define META_IMPLEMENT_PROPERTY(...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_PROPERTY,, META_NS::InterfaceInfo {}, __VA_ARGS__))
#define META_IMPLEMENT_READONLY_PROPERTY(...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_READONLY_PROPERTY,, META_NS::InterfaceInfo {}, __VA_ARGS__))
#define META_IMPLEMENT_ARRAY_PROPERTY(...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_ARRAY_PROPERTY, Array, META_NS::InterfaceInfo {}, __VA_ARGS__))
#define META_IMPLEMENT_READONLY_ARRAY_PROPERTY(...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_READONLY_ARRAY_PROPERTY, Array, META_NS::InterfaceInfo {}, __VA_ARGS__))

/**
 * @brief Same as META_IMPLEMENT_PROPERTY but first parameter tells which interface introduced this property.
 */
#define META_IMPLEMENT_INTERFACE_PROPERTY(intf, ...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_PROPERTY,, intf::INTERFACE_INFO, __VA_ARGS__))
#define META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(intf, ...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_READONLY_PROPERTY,, intf::INTERFACE_INFO, __VA_ARGS__))
#define META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(intf, ...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_ARRAY_PROPERTY, Array, intf::INTERFACE_INFO, __VA_ARGS__))
#define META_IMPLEMENT_INTERFACE_READONLY_ARRAY_PROPERTY(intf, ...) \
    META_EXPAND(META_GET_PROPERTY(META_DEFINE_READONLY_ARRAY_PROPERTY, Array, intf::INTERFACE_INFO, __VA_ARGS__))

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

#define META_DEFINE_EVENT_VAR(intf, type, name)                                                                  \
    mutable BASE_NS::shared_ptr<::META_NS::EventImpl<type>> metaEvent##name##_ =                                 \
        this->template RegisterStaticEventMetadata<type>(                                                        \
            intf, #name, [] { return ::META_NS::IEvent::Ptr(CreateShared<::META_NS::EventImpl<type>>(#name)); }, \
            [](auto me) { /*workaround for older msvc not supporting decltype(this) without capture*/            \
                return [](void* self, const ::META_NS::IEvent::Ptr& p) {                                         \
                    if (p->GetCallableUid() == type::UID) {                                                      \
                        /* notice this is dangerous if same UID used for many different event types*/            \
                        static_cast<decltype(me)>(self)->metaEvent##name##_ =                                    \
                            static_pointer_cast<::META_NS::EventImpl<type>>(p);                                  \
                        return true;                                                                             \
                    }                                                                                            \
                    return false;                                                                                \
                };                                                                                               \
            }(this));

#define META_DEFINE_EVENT(intf, itype, etype, name)                      \
    META_DEFINE_EVENT_VAR(intf, etype, name)                             \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name() const override  \
    {                                                                    \
        CORE_ASSERT_MSG(metaEvent##name##_, "Metadata not initialized"); \
        return metaEvent##name##_;                                       \
    }                                                                    \
    META_EVENT_TYPED_IMPL(itype, name)

#define META_GET_EVENT3(intf, itype, etype, name) META_DEFINE_EVENT(intf, itype, etype, name)
#define META_GET_EVENT2(intf, etype, name) META_DEFINE_EVENT(intf, etype, etype, name)
#define META_GET_EVENT(...) \
    META_EXPAND(META_GET_MACRO4_IMPL(__VA_ARGS__, META_GET_EVENT3, META_GET_EVENT2)(__VA_ARGS__))

/**
 * @brief Implement event that was introduced using META_PROPERTY in interface.
 *        This will add the event to static metadata and generates code to construct
 *        it when the object is constructed.
 */
#define META_IMPLEMENT_EVENT(...) META_EXPAND(META_GET_EVENT(META_NS::InterfaceInfo {}, __VA_ARGS__))
/**
 * @brief Same as META_IMPLEMENT_EVENT but first parameter tells which interface introduced this event.
 */
#define META_IMPLEMENT_INTERFACE_EVENT(intf, ...) META_EXPAND(META_GET_EVENT(intf::INTERFACE_INFO, __VA_ARGS__))

#define META_FORWARD_EVENT(type, name, forwarder)                       \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name() const override \
    {                                                                   \
        return forwarder;                                               \
    }                                                                   \
    META_EVENT_TYPED_IMPL(type, name)

#define META_FORWARD_EVENT_CLASS(type, name, targetClass)               \
    BASE_NS::shared_ptr<::META_NS::IEvent> Event##name() const override \
    {                                                                   \
        return targetClass::Event##name();                              \
    }                                                                   \
    ::META_NS::Event<type> name() const                                 \
    {                                                                   \
        return targetClass::Event##name();                              \
    }

#define META_DEFINE_FUNCTION_VAR(intf, class, func, ...)                                                 \
    mutable ::META_NS::IFunction::Ptr metaFunc##func##_ = this->RegisterStaticFunctionMetadata(          \
        intf, #func,                                                                                     \
        [](auto me) {                                                                                    \
            return [](void* self) {                                                                      \
                auto ccontext = []() {                                                                   \
                    ::BASE_NS::string_view arr[] = { "", __VA_ARGS__ };                                  \
                    return CreateCallContext(&class ::func, ::META_NS::ParamNameToView(arr));            \
                };                                                                                       \
                return ::META_NS::IFunction::Ptr(::META_NS::CreateFunction(                              \
                    #func, static_cast<decltype(me)>(self), &class ::func##MetaImpl, ccontext));         \
            };                                                                                           \
        }(this),                                                                                         \
        []() {                                                                                           \
            /*cannot have zero size array, so we add something in front and ParamNameToView removes it*/ \
            BASE_NS::string_view arr[] = { "", __VA_ARGS__ };                                            \
            return ::META_NS::CreateCallContext(&class ::func, ::META_NS::ParamNameToView(arr));         \
        });

#define META_DEFINE_IMPL_FUNCTION(class, func)                       \
    void func##MetaImpl(const ::META_NS::ICallContext::Ptr& context) \
    {                                                                \
        ::META_NS::CallFunction(context, this, &class ::func);       \
    }

#define META_IMPLEMENT_PLAIN_FUNCTION(class, func, ...) \
    META_DEFINE_FUNCTION_VAR(META_NS::InterfaceInfo {}, class, func, __VA_ARGS__)

#define META_IMPLEMENT_FUNCTION(class, func, ...)           \
    META_IMPLEMENT_PLAIN_FUNCTION(class, func, __VA_ARGS__) \
    META_DEFINE_IMPL_FUNCTION(class, func)

#define META_IMPLEMENT_INTERFACE_PLAIN_FUNCTION(interface, class, func, ...) \
    META_DEFINE_FUNCTION_VAR(interface::INTERFACE_INFO, class, func, __VA_ARGS__)

#define META_IMPLEMENT_INTERFACE_FUNCTION(interface, class, func, ...)           \
    META_IMPLEMENT_INTERFACE_PLAIN_FUNCTION(interface, class, func, __VA_ARGS__) \
    META_DEFINE_IMPL_FUNCTION(class, func)

#endif
