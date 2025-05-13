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

#ifndef META_INTERFACE_STATIC_OBJECT_METADATA_H
#define META_INTERFACE_STATIC_OBJECT_METADATA_H

#include <base/containers/shared_ptr.h>

#include <meta/base/namespace.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

class IOwner;
class IAny;

struct StaticMetadata;

namespace Internal {
using MetadataCtor = BASE_NS::shared_ptr<CORE_NS::IInterface>(
    const BASE_NS::shared_ptr<IOwner>&, const StaticMetadata& data);
using MetaValue = BASE_NS::shared_ptr<IAny>();
enum class StaticMetaFlag : uint8_t { FORWARD = 1 };
enum class PropertyFlag : uint8_t { READONLY = 8 };
} // namespace Internal

enum class MetadataType : uint8_t { UNKNOWN = 0, PROPERTY = 1, EVENT = 2, FUNCTION = 4 };

inline MetadataType operator|(MetadataType a, MetadataType b)
{
    return MetadataType(uint8_t(a) | uint8_t(b));
}

/// Single static metadata entry
struct StaticMetadata {
    const MetadataType type {};
    const InterfaceInfo interfaceInfo {};
    const char* const name {};
    Internal::MetadataCtor* const create {};
    Internal::MetaValue* const runtimeValue {};
    const void* const data {};
    const uint8_t flags {};
};

/// Static metadata for single object class
struct StaticObjectMetadata {
    const ClassInfo* classInfo {};
    const StaticObjectMetadata* baseclass {}; // direct base class
    const StaticObjectMetadata* aggregate {}; // aggregate "base" class
    const StaticMetadata* metadata {};
    const size_t size {};
};

META_END_NAMESPACE()

#endif
