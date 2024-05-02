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

#ifndef META_INTERFACE_IMETA_OBJECT_LIB_H
#define META_INTERFACE_IMETA_OBJECT_LIB_H

#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

CORE_BEGIN_NAMESPACE()
struct SyncApi;
CORE_END_NAMESPACE()

META_BEGIN_NAMESPACE()

class IObjectRegistry;
class ITaskQueueRegistry;
class IBindingState;
class IAnimationController;

/**
 * @brief Meta object library global access point for globally used objects.
 */
class IMetaObjectLib : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetaObjectLib, "6f4ba852-3a00-4bbd-82bf-c36fb657992e")
public:
    virtual IObjectRegistry& GetObjectRegistry() const = 0;
    virtual ITaskQueueRegistry& GetTaskQueueRegistry() const = 0;
    virtual BASE_NS::shared_ptr<IAnimationController> GetAnimationController() const = 0;
    virtual const CORE_NS::SyncApi& GetSyncApi() const = 0;
};

/**
 * @brief Get global IMetaObjectLib instance.
 */
inline IMetaObjectLib& GetMetaObjectLib()
{
    auto* instance = CORE_NS::GetInstance<IMetaObjectLib>(IMetaObjectLib::UID);
    if (instance) {
        return *instance;
    }
    CORE_LOG_F("Meta Object Lib not initialized");
    CORE_ASSERT(false);
    return *instance;
}

META_END_NAMESPACE()

#endif
