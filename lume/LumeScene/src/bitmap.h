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

#ifndef SCENE_SRC_BITMAP_H
#define SCENE_SRC_BITMAP_H

#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_bitmap.h>
#include <shared_mutex>

#include <render/device/intf_gpu_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

class Bitmap : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IBitmap, IRenderTarget, IRenderResource,
                   IEcsResource, META_NS::IDynamicResource, META_NS::INamed> {
    META_OBJECT(Bitmap, ClassId::Bitmap, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(SCENE_NS::IBitmap, BASE_NS::Math::UVec2, Size, BASE_NS::Math::UVec2(0, 0),
        META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_EVENT_DATA(META_NS::IDynamicResource, META_NS::IOnChanged, OnResourceChanged)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnResourceChanged)

    bool SetRenderHandle(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle,
        CORE_NS::EntityReference ent) override;
    RENDER_NS::RenderHandleReference GetRenderHandle() const override;
    CORE_NS::Entity GetEntity() const override;

    void Refresh() override {}

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }

private:
    mutable std::shared_mutex mutex_;
    RENDER_NS::RenderHandleReference handle_;
    CORE_NS::EntityReference entity_;
};

SCENE_END_NAMESPACE()

#endif