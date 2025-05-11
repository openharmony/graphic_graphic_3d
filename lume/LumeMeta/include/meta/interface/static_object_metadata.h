/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: Static Object Meta data
 * Author: Mikael Kilpeläinen
 * Create: 2023-03-27
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
