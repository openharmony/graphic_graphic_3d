/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_MACROS_H
#define CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_MACROS_H

#ifndef BEGIN_PROPERTY
// Include actual macros, with the core property metadatas.
#include "PropertyTools/core_metadata.inl"
#endif

#ifdef IMPLEMENT_MANAGER
// Override the component declaration macros for "inline include" metadata definitions.
#undef BEGIN_COMPONENT
#undef ARRAY_VALUE
#undef VALUE
#undef DEFINE_PROPERTY
#undef DEFINE_BITFIELD_PROPERTY
#undef DEFINE_ARRAY_PROPERTY
#undef BEGIN_COMPONENT_MANAGER
#undef END_COMPONENT_MANAGER
#undef END_COMPONENT
#undef END_COMPONENT_EXT
#undef END_COMPONENT2

#define BEGIN_COMPONENT(a, b)
#define ARRAY_VALUE(...)
#define VALUE(val)
#define DEFINE_PROPERTY(type, name, displayname, flags, value) DECL_PROPERTY2(COMPTYPE, name, displayname, flags)
#define DEFINE_BITFIELD_PROPERTY(type, name, displayname, flags, value, enumeration) \
    DECL_BITFIELD_PROPERTY(COMPTYPE, name, displayname, flags, enumeration)
#define DEFINE_ARRAY_PROPERTY(type, count, name, displayname, flags, value) \
    DECL_PROPERTY2(COMPTYPE, name, displayname, flags)
#define BEGIN_COMPONENT_MANAGER(a, b, c)
#define END_COMPONENT_MANAGER(a, b, c)
#define END_COMPONENT(a, b, c)
#define END_COMPONENT_EXT(a, b, c, d)
#endif

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_MACROS_H
