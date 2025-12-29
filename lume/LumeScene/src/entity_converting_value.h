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

#ifndef SCENE_SRC_ENTITY_CONVERTING_VALUE_H
#define SCENE_SRC_ENTITY_CONVERTING_VALUE_H

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_render_resource.h>

#include <3d/ecs/components/render_handle_component.h>

#include "converting_value.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()

template<typename Interface>
CORE_NS::Entity ConvertPtrToTarget(const typename Interface::Ptr& v)
{
    return GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(v));
}

template<typename Interface>
typename Interface::Ptr ConvertPtrToSource(
    META_NS::IAny& any, const CORE_NS::Entity& v, const IInternalScene::Ptr& scene, META_NS::ObjectId id)
{
    auto p = META_NS::GetPointer<Interface>(any);
    if (scene) {
        if (CORE_NS::EntityUtil::IsValid(v)) {
            if (p && (GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(p)) != v)) {
                p = nullptr;
            }
            if (!p) {
                scene->RunDirectlyOrInTask([&] { p = interface_pointer_cast<Interface>(scene->CreateObject(id, v)); });
            }
        } else {
            p = nullptr;
        }
    }
    return p;
}

/// Interface pointer to entity converter with fixed Entity<->ObjectId mapping
template<typename Interface>
struct InterfacePtrEntityConverter {
    using SourceType = typename Interface::Ptr;
    using TargetType = CORE_NS::Entity;

    InterfacePtrEntityConverter(IInternalScene::Ptr scene, META_NS::ObjectId id) : scene_(scene), id_(id) {}

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        return ConvertPtrToSource<Interface>(any, v, scene_.lock(), id_);
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return ConvertPtrToTarget<Interface>(v);
    }

protected:
    IInternalScene::WeakPtr scene_;
    META_NS::ObjectId id_;
};

/// Interface pointer to entity converter with dynamic Entity<->ObjectId mapping
template<typename Interface>
struct DynamicInterfacePtrEntityConverter {
    using SourceType = typename Interface::Ptr;
    using TargetType = CORE_NS::Entity;

    explicit DynamicInterfacePtrEntityConverter(IInternalScene::Ptr scene) : scene_(scene) {}

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        const auto scene = scene_.lock();
        META_NS::ObjectId id {};
        if (scene) {
            id = GetObjectId(*(scene->GetEcsContext().GetNativeEcs()), v);
        }
        return ConvertPtrToSource<Interface>(any, v, scene, id);
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return ConvertPtrToTarget<Interface>(v);
    }

protected:
    /**
     * @brief To be overridden by implementations to provide dynamic entity<->ObjectId mapping.
     */
    virtual META_NS::ObjectId GetObjectId(const CORE_NS::IEcs& ecs, const CORE_NS::Entity& e) const = 0;
    IInternalScene::WeakPtr scene_;
};

template<typename Interface>
using InterfacePtrEntityValue = ConvertingValue<InterfacePtrEntityConverter<Interface>>;

// render resource to entity converter
template<typename Interface>
struct RenderResourceConverter {
    using SourceType = typename Interface::Ptr;
    using TargetType = CORE_NS::EntityReference;

    RenderResourceConverter(IInternalScene::Ptr scene, META_NS::ObjectId id) : scene_(scene), id_(id) {}

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<Interface>(any);
        if (auto scene = scene_.lock()) {
            if (v) {
                p = scene->RunDirectlyOrInTask([&] { return ObjectWithRenderHandle<Interface>(scene, v, p, id_); });
            } else {
                p = nullptr;
            }
        }
        return p;
    }

    TargetType ConvertToTarget(const SourceType& v) const
    {
        CORE_NS::EntityReference ent;
        if (auto scene = scene_.lock()) {
            ent = scene->RunDirectlyOrInTask([&] { return HandleFromRenderResource<Interface>(scene, v); });
        }
        return ent;
    }

protected:
    IInternalScene::WeakPtr scene_;
    META_NS::ObjectId id_;
};

template<typename Interface>
using RenderResourceValue = ConvertingValue<RenderResourceConverter<Interface>>;

SCENE_END_NAMESPACE()

#endif
