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

#include "engine_factory.h"

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_info.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::Uid;

// Engine factory
IEngine::Ptr CreateEngine(EngineCreateInfo const& createInfo);

IEngine::Ptr EngineFactory::Create(const EngineCreateInfo& engineCreateInfo)
{
    return IEngine::Ptr { CreateEngine(engineCreateInfo) };
}

Uid EngineFactory::GetClassUid() const
{
    return UID_ENGINE_FACTORY;
}

string_view EngineFactory::GetClassName() const
{
    return "EngineFactory";
}

const IInterface* EngineFactory::GetInterface(const Uid& uid) const
{
    if ((uid == IEngineFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<const IEngineFactory*>(this);
    }
    if (uid == IClassInfo::UID) {
        return static_cast<const IClassInfo*>(this);
    }
    return nullptr;
}

IInterface* EngineFactory::GetInterface(const Uid& uid)
{
    if ((uid == IEngineFactory::UID) || (uid == IInterface::UID)) {
        return static_cast<IEngineFactory*>(this);
    }
    if (uid == IClassInfo::UID) {
        return static_cast<IClassInfo*>(this);
    }
    return nullptr;
}

void EngineFactory::Ref() {}

void EngineFactory::Unref() {}
CORE_END_NAMESPACE()
