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

#include "class_registry.h"

#include <base/util/uid_util.h>

#include "meta/base/interface_utils.h"

META_BEGIN_NAMESPACE()

void ClassRegistry::Clear()
{
    std::unique_lock lock { mutex_ };
    objectFactories_.clear();
}

bool ClassRegistry::Unregister(const IObjectFactory::Ptr& fac)
{
    if (!fac) {
        CORE_LOG_E("ClassRegistry: Cannot unregister a null object factory");
        return false;
    }
    size_t erased = 0;
    {
        std::unique_lock lock { mutex_ };
        erased = objectFactories_.erase(fac->GetClassInfo());
    }
    if (erased) {
        onUnregistered_->Invoke({ fac });
        return true;
    }
    return false;
}

bool ClassRegistry::Register(const IObjectFactory::Ptr& fac)
{
    if (!fac) {
        CORE_LOG_E("ClassRegistry: Cannot register a null object factory");
        return false;
    }
    {
        std::unique_lock lock { mutex_ };
        auto& info = fac->GetClassInfo();
        auto& i = objectFactories_[info];
        if (i) {
            CORE_LOG_W("ClassRegistry: Cannot register a class that was already registered [name=%s, uid=%s]",
                info.Name().data(), info.Id().ToString().c_str());
            return false;
        }
        i = fac;
    }
    onRegistered_->Invoke({ fac });
    return true;
}

IObjectFactory::ConstPtr ClassRegistry::GetObjectFactory(const BASE_NS::Uid& uid) const
{
    std::shared_lock lock { mutex_ };
    auto it = objectFactories_.find(uid);
    return it != objectFactories_.end() ? it->second : nullptr;
}

BASE_NS::string ClassRegistry::GetClassName(BASE_NS::Uid uid) const
{
    std::shared_lock lock { mutex_ };
    auto it = objectFactories_.find(uid);
    return it != objectFactories_.end() ? BASE_NS::string(it->second->GetClassInfo().Name())
                                        : BASE_NS::string("Unknown class id [") + BASE_NS::to_string(uid) + "]";
}

BASE_NS::vector<IClassInfo::ConstPtr> ClassRegistry::GetAllTypes(
    ObjectCategoryBits category, bool strict, bool excludeDeprecated) const
{
    std::shared_lock lock { mutex_ };
    BASE_NS::vector<IClassInfo::ConstPtr> infos;
    for (auto&& v : objectFactories_) {
        const auto& factory = v.second;
        if (excludeDeprecated && (factory->GetClassInfo().category & ObjectCategoryBits::DEPRECATED)) {
            // Omit DEPRECATED classes if excludeDeprecated flag is true
            continue;
        }
        if (CheckCategoryBits(factory->GetClassInfo().category, category, strict)) {
            infos.emplace_back(factory);
        }
    }
    return infos;
}

BASE_NS::vector<IClassInfo::ConstPtr> ClassRegistry::GetAllTypes(
    const BASE_NS::vector<BASE_NS::Uid>& interfaceUids, bool strict, bool excludeDeprecated) const
{
    std::shared_lock lock { mutex_ };
    BASE_NS::vector<IClassInfo::ConstPtr> infos;
    for (auto&& v : objectFactories_) {
        const auto& factory = v.second;
        if (factory->GetClassInfo().category & ObjectCategoryBits::INTERNAL) {
            // Omit classes with INTERNAL flag from the list
            continue;
        }
        if (excludeDeprecated && (factory->GetClassInfo().category & ObjectCategoryBits::DEPRECATED)) {
            // Omit DEPRECATED classes if excludeDeprecated flag is true
            continue;
        }
        if (CheckInterfaces(factory->GetClassInterfaces(), interfaceUids, strict)) {
            infos.push_back(factory);
        }
    }
    return infos;
}

META_END_NAMESPACE()
