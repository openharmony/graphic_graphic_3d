/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object type information.
 * Author: Jani Kattelus
 * Create: 2022-10-11
 */

#ifndef META_INTERFACE_OBJECT_TYPE_INFO_H
#define META_INTERFACE_OBJECT_TYPE_INFO_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/plugin/intf_plugin.h>

#include <meta/interface/intf_object_factory.h>

META_BEGIN_NAMESPACE()

using GetFactoryType = IObjectFactory::Ptr (*)();

/**
 * @brief Type information for an IObject, to be used when registering a new object type.
 */
struct ObjectTypeInfo : public CORE_NS::ITypeInfo {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr BASE_NS::Uid UID { "57b61777-e747-4281-9a06-a17414a07206" };
    const GetFactoryType GetFactory = nullptr;
};

static bool operator==(const ObjectTypeInfo& lhs, const ObjectTypeInfo& rhs)
{
    if (lhs.GetFactory && rhs.GetFactory) {
        auto lhsF = lhs.GetFactory();
        auto rhsF = rhs.GetFactory();
        if (lhsF && rhsF) {
            return lhsF->GetClassInfo() == rhsF->GetClassInfo();
        }
    }
    return false;
}

META_END_NAMESPACE()

#endif
