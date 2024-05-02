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

#include "meta_object_lib.h"

#include <shared_mutex>

#include <meta/interface/animation/builtin_animations.h>

META_BEGIN_NAMESPACE()

using CORE_NS::MutexHandle;
using CORE_NS::MutexType;

#if defined(__ANDROID__)
using InternalMutexType = std::mutex;
#else
// std::mutex on vc2017 is 80 bytes and much slower than std::shared_mutex
using InternalMutexType = std::shared_mutex;
#endif

constexpr bool USE_IN_PLACE_MUTEX =
    sizeof(InternalMutexType) <= sizeof(MutexHandle::storage) && alignof(InternalMutexType) <= alignof(MutexHandle);

static void CreateMutex(MutexType, MutexHandle& handle)
{
    // do we have suitable storage for constructing mutex in-place
    if constexpr (USE_IN_PLACE_MUTEX) {
        new (handle.storage) InternalMutexType;
    } else {
        handle.ptr = new InternalMutexType;
    }
}

static void DestroyMutex(MutexHandle& handle)
{
    if constexpr (USE_IN_PLACE_MUTEX) {
        static_cast<InternalMutexType*>(static_cast<void*>(handle.storage))->~InternalMutexType();
    } else {
        delete static_cast<InternalMutexType*>(handle.ptr);
    }
}

static bool LockMutex(MutexHandle& handle)
{
    if constexpr (USE_IN_PLACE_MUTEX) {
        static_cast<InternalMutexType*>(static_cast<void*>(handle.storage))->lock();
    } else {
        static_cast<InternalMutexType*>(handle.ptr)->lock();
    }
    return true;
}

static bool UnlockMutex(MutexHandle& handle)
{
    if constexpr (USE_IN_PLACE_MUTEX) {
        static_cast<InternalMutexType*>(static_cast<void*>(handle.storage))->unlock();
    } else {
        static_cast<InternalMutexType*>(handle.ptr)->unlock();
    }
    return true;
}

static uint64_t GetThreadId()
{
    thread_local const char variable {};
    return reinterpret_cast<uint64_t>(&variable);
}

namespace Internal {
void RegisterEntities(IObjectRegistry& registry);
void UnRegisterEntities(IObjectRegistry& registry);
void RegisterValueSerializers(IObjectRegistry& registry);
void UnRegisterValueSerializers(IObjectRegistry& registry);
void RegisterAnys(IObjectRegistry& registry);
void UnRegisterAnys(IObjectRegistry& registry);
void RegisterEngineTypes(IObjectRegistry& registry);
void UnRegisterEngineTypes(IObjectRegistry& registry);
} // namespace Internal

const CORE_NS::IInterface* MetaObjectLib::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == IMetaObjectLib::UID) {
        return this;
    }
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* MetaObjectLib::GetInterface(const BASE_NS::Uid& uid)
{
    const auto* p = this;
    return const_cast<CORE_NS::IInterface*>(p->MetaObjectLib::GetInterface(uid));
}

MetaObjectLib::MetaObjectLib()
    : sapi_ { { CreateMutex, DestroyMutex, LockMutex, UnlockMutex }, GetThreadId }
{
    if (USE_IN_PLACE_MUTEX) {
        CORE_LOG_D("Using in-place mutex");
    } else {
        CORE_LOG_D("Not using in-place mutex");
    }
}

void MetaObjectLib::Initialize()
{
    registry_ = new ObjectRegistry;
    Internal::RegisterAnys(*registry_);
    Internal::RegisterEntities(*registry_);
    Internal::RegisterValueSerializers(*registry_);
    Internal::RegisterEngineTypes(*registry_);
}

void MetaObjectLib::Uninitialize()
{
    Internal::UnRegisterEngineTypes(*registry_);
    Internal::UnRegisterValueSerializers(*registry_);
    Internal::UnRegisterEntities(*registry_);
    Internal::UnRegisterAnys(*registry_);
}

MetaObjectLib::~MetaObjectLib()
{
    animationController_.reset();
    registry_->Purge();
    delete registry_;
}

IObjectRegistry& MetaObjectLib::GetObjectRegistry() const
{
    return *registry_;
}

ITaskQueueRegistry& MetaObjectLib::GetTaskQueueRegistry() const
{
    return *static_cast<ITaskQueueRegistry*>(registry_);
}

IAnimationController::Ptr MetaObjectLib::GetAnimationController() const
{
    std::call_once(animInit_, [&] {
        auto iregistry = static_cast<IObjectRegistry*>(registry_);
        animationController_ = iregistry->Create<IAnimationController>(ClassId::AnimationController);
    });
    return animationController_;
}

const CORE_NS::SyncApi& MetaObjectLib::GetSyncApi() const
{
    return sapi_;
}

META_END_NAMESPACE()
