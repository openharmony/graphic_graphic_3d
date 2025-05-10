/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_CORE_PROPERTY_TOOLS_PROPERTY_MACROS_H
#define API_CORE_PROPERTY_TOOLS_PROPERTY_MACROS_H

#ifndef NO_MEMBERS
// Include actual macros, with the core property metadatas.
#include <core/property_tools/core_metadata.inl>
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

// Helpers which can be used when defining a component manager with the help of component_struct_macros.h.
#define BEGIN_COMPONENT(a, b)
#define ARRAY_VALUE(...)
#define VALUE(val)
#define DEFINE_PROPERTY(type, name, displayname, flags, value) MEMBER_PROPERTY(name, displayname, flags),
#define DEFINE_BITFIELD_PROPERTY(type, name, displayname, flags, value, enumeration) \
    BITFIELD_MEMBER_PROPERTY(name, displayname, flags, enumeration),
#define DEFINE_ARRAY_PROPERTY(type, count, name, displayname, flags, value) MEMBER_PROPERTY(name, displayname, flags),
#define BEGIN_COMPONENT_MANAGER(a, b, c)
#define END_COMPONENT_MANAGER(a, b, c)
#define END_COMPONENT(a, b, c)
#define END_COMPONENT_EXT(a, b, c, d)
#define BEGIN_PROPERTY(dataType, storageName) \
    using COMPTYPE = dataType;                \
    static constexpr CORE_NS::Property storageName[] = {
#define END_PROPERTY() }
#endif

#endif // API_CORE_PROPERTY_TOOLS_PROPERTY_MACROS_H
