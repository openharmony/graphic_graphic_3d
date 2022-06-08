/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "engine_factory.h"

#include <core/implementation_uids.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::vector;

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
    if (uid == IEngineFactory::UID) {
        return static_cast<const IEngineFactory*>(this);
    } else if (uid == IClassInfo::UID) {
        return static_cast<const IClassInfo*>(this);
    }
    return nullptr;
}

IInterface* EngineFactory::GetInterface(const Uid& uid)
{
    if (uid == IEngineFactory::UID) {
        return static_cast<IEngineFactory*>(this);
    } else if (uid == IClassInfo::UID) {
        return static_cast<IClassInfo*>(this);
    }
    return nullptr;
}

void EngineFactory::Ref() {}

void EngineFactory::Unref() {}
CORE_END_NAMESPACE()
