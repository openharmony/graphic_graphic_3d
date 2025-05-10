/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: interface macros
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-13
 */

#ifndef META_INTERFACE_INTERFACE_MACROS_H
#define META_INTERFACE_INTERFACE_MACROS_H

#include <base/containers/shared_ptr.h>

#include <meta/base/namespace.h>
#include <meta/interface/event.h>
#include <meta/interface/metadata_query.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/property.h>

#define META_READONLY_PROPERTY_TYPED_IMPL(type, name)                   \
    META_NS::ConstProperty<type> name() const noexcept                  \
    {                                                                   \
        return META_NS::ConstProperty<type> { this->Property##name() }; \
    }

#define META_PROPERTY_TYPED_IMPL(type, name)                       \
    META_NS::Property<type> name() noexcept                        \
    {                                                              \
        return META_NS::Property<type> { this->Property##name() }; \
    }

#define META_EVENT_TYPED_IMPL(type, name)                                 \
    ::META_NS::Event<type> name() const                                   \
    {                                                                     \
        return Event##name(META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST); \
    }

/**
 * @brief Define read-only property with given type and name.
 *        Creates only const version.
 */
#define META_READONLY_PROPERTY(type, name)                                            \
    virtual BASE_NS::shared_ptr<const META_NS::IProperty> Property##name() const = 0; \
    META_READONLY_PROPERTY_TYPED_IMPL(type, name)

/**
 * @brief Define property with given type and name.
 *        Creates overloads for const and non-const versions.
 */
#define META_PROPERTY(type, name)                                         \
    META_READONLY_PROPERTY(type, name)                                    \
    virtual BASE_NS::shared_ptr<META_NS::IProperty> Property##name() = 0; \
    META_PROPERTY_TYPED_IMPL(type, name)

#define META_READONLY_ARRAY_PROPERTY_TYPED_IMPL(type, name)                  \
    META_NS::ConstArrayProperty<type> name() const noexcept                  \
    {                                                                        \
        return META_NS::ConstArrayProperty<type> { this->Property##name() }; \
    }

#define META_ARRAY_PROPERTY_TYPED_IMPL(type, name)                      \
    META_NS::ArrayProperty<type> name() noexcept                        \
    {                                                                   \
        return META_NS::ArrayProperty<type> { this->Property##name() }; \
    }

/**
 * @brief Define read-only property with given type and name.
 *        Creates only const version.
 */
#define META_READONLY_ARRAY_PROPERTY(type, name)                                      \
    virtual BASE_NS::shared_ptr<const META_NS::IProperty> Property##name() const = 0; \
    META_READONLY_ARRAY_PROPERTY_TYPED_IMPL(type, name)

/**
 * @brief Define property with given type and name.
 *        Creates overloads for const and non-const versions.
 */
#define META_ARRAY_PROPERTY(type, name)                                   \
    META_READONLY_ARRAY_PROPERTY(type, name)                              \
    virtual BASE_NS::shared_ptr<META_NS::IProperty> Property##name() = 0; \
    META_ARRAY_PROPERTY_TYPED_IMPL(type, name)

/**
 * @brief Define event with given type and name.
 */
#define META_EVENT(type, name)                                                                      \
    virtual ::BASE_NS::shared_ptr<::META_NS::IEvent> Event##name(META_NS::MetadataQuery) const = 0; \
    META_EVENT_TYPED_IMPL(type, name)

#endif
