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

#ifndef META_OBJECT_LIB_H
#define META_OBJECT_LIB_H
#include <mutex>

#include <meta/base/interface_macros.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/threading/primitive_api.h>

#include "object_registry.h"

META_BEGIN_NAMESPACE()

class MetaObjectLib : public IMetaObjectLib {
public:
    META_IMPLEMENT_REF_COUNT_NO_ONDESTROY();

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    MetaObjectLib();
    ~MetaObjectLib() override;

    META_NO_COPY_MOVE(MetaObjectLib)

    void Initialize();
    void Uninitialize();

    IObjectRegistry& GetObjectRegistry() const override;
    ITaskQueueRegistry& GetTaskQueueRegistry() const override;
    IAnimationController::Ptr GetAnimationController() const override;
    const CORE_NS::SyncApi& GetSyncApi() const override;

private:
    mutable std::once_flag animInit_;

    // the registry is still requested when destroyed, so we keep plain pointer to circumvent that.
    ObjectRegistry* registry_ {};
    mutable IAnimationController::Ptr animationController_;
    const CORE_NS::SyncApi sapi_;
};

META_END_NAMESPACE()
#endif