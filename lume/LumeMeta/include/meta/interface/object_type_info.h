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
