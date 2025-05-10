/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Meta Object Library
 */

#ifndef META_INTERFACE_IMETA_OBJECT_LIB_H
#define META_INTERFACE_IMETA_OBJECT_LIB_H

#include <core/log.h>
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
    // virtual IBindingState& GetBindingState() const = 0;
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
    abort();
}

META_END_NAMESPACE()

#endif
