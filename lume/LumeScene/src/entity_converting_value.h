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

// Interface pointer to entity converter
template<typename Interface>
struct InterfacePtrEntityConverter {
    using SourceType = typename Interface::Ptr;
    using TargetType = CORE_NS::Entity;

    InterfacePtrEntityConverter(IInternalScene::Ptr scene, META_NS::ObjectId id) : scene_(scene), id_(id) {}

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<Interface>(any);
        if (auto scene = scene_.lock()) {
            if (CORE_NS::EntityUtil::IsValid(v)) {
                auto f = scene->AddTask([&] { return scene->GetEcsContext().GetEcsObject(v); });
                auto obj = f.GetResult();
                if (p) {
                    if (auto i = interface_cast<IEcsObjectAccess>(p)) {
                        if (i->GetEcsObject() != obj) {
                            p = nullptr;
                        }
                    }
                }
                if (!p) {
                    p = META_NS::GetObjectRegistry().Create<Interface>(id_);
                    if (auto i = interface_cast<IEcsObjectAccess>(p)) {
                        i->SetEcsObject(obj);
                    }
                }
            } else {
                p = nullptr;
            }
        }
        return p;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        CORE_NS::Entity ent;
        if (auto scene = scene_.lock()) {
            if (v) {
                if (auto i = interface_cast<IEcsObjectAccess>(v)) {
                    ent = i->GetEcsObject()->GetEntity();
                }
            }
        }
        return ent;
    }

    IInternalScene::WeakPtr scene_;
    META_NS::ObjectId id_;
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
                p = scene->AddTask([&] { return ObjectWithRenderHandle<Interface>(scene, v, p, id_); }).GetResult();
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
            ent = scene->AddTask([&] { return HandleFromRenderResource<Interface>(scene, v); }).GetResult();
        }
        return ent;
    }

    IInternalScene::WeakPtr scene_;
    META_NS::ObjectId id_;
};

template<typename Interface>
using RenderResourceValue = ConvertingValue<RenderResourceConverter<Interface>>;

SCENE_END_NAMESPACE()

#endif
