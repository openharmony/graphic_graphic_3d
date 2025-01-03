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

#ifndef SCENE_SRC_MESH_SHADER_H
#define SCENE_SRC_MESH_SHADER_H

#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_shader.h>
#include <shared_mutex>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

class Shader : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IShader, IRenderResource, IEcsResource,
                   IGraphicsState, META_NS::IDynamicResource, META_NS::INamed> {
    META_OBJECT(Shader, ClassId::Shader, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(IShader, BASE_NS::string, Uri)
    META_STATIC_EVENT_DATA(META_NS::IDynamicResource, META_NS::IOnChanged, OnResourceChanged)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_READONLY_PROPERTY(BASE_NS::string, Uri)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnResourceChanged)

    BASE_NS::string GetName() const override;

    bool SetRenderHandle(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle,
        CORE_NS::EntityReference ent) override;
    RENDER_NS::RenderHandleReference GetRenderHandle() const override;
    CORE_NS::EntityReference GetEntity() const override;

    bool SetGraphicsState(CORE_NS::EntityReference) override;
    CORE_NS::EntityReference GetGraphicsState() const override;

    Future<bool> LoadShader(const IScene::Ptr&, BASE_NS::string_view uri) override;

    void Refresh() override {}

    Future<CullModeFlags> GetCullMode() const override;
    Future<bool> SetCullMode(CullModeFlags) override;
    Future<bool> IsBlendEnabled() const override;
    Future<bool> EnableBlend(bool) override;

private:
    bool SetRenderHandleImpl(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference);
    IInternalScene::Ptr GetScene() const;

    RENDER_NS::RenderHandleReference GetGraphicsStateHandle() const;
    void UpdateGraphicsState(
        const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference h, const RENDER_NS::GraphicsState& gs);

private:
    mutable std::shared_mutex mutex_;
    RENDER_NS::RenderHandleReference handle_;
    CORE_NS::EntityReference entity_;
    CORE_NS::EntityReference graphicsState_;
    RENDER_NS::RenderHandleReference graphicsStateHandle_;
    IInternalScene::WeakPtr scene_;
};

SCENE_END_NAMESPACE()

#endif