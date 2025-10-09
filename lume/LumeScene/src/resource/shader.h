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

#ifndef SCENE_SRC_RESOURCE_SHADER_H
#define SCENE_SRC_RESOURCE_SHADER_H

#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/types.h>
#include <shared_mutex>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

#include "render_resource.h"

SCENE_BEGIN_NAMESPACE()

class GraphicsState : public META_NS::IntroduceInterfaces<RenderResource, IGraphicsState> {
    META_OBJECT_NO_CLASSINFO(GraphicsState, IntroduceInterfaces)
public:
    bool SetGraphicsState(RENDER_NS::RenderHandleReference) override;
    RENDER_NS::RenderHandleReference GetGraphicsState() const override;

    RENDER_NS::GraphicsState CreateGraphicsState(const IRenderContext::Ptr& context,
        RENDER_NS::RenderHandleReference state, BASE_NS::string_view defaultRenderSlot);
    bool UpdateGraphicsState(
        const IRenderContext::Ptr &context, const RENDER_NS::GraphicsState &gs, BASE_NS::string_view renderSlot);
    RENDER_NS::GraphicsState CreateNewGraphicsState(const IRenderContext::Ptr& context, bool blend);
    RENDER_NS::GraphicsState GetGraphicsState(const IRenderContext::Ptr& context) const;

protected:
    BASE_NS::string path_;
    BASE_NS::string variant_;
    RENDER_NS::RenderHandleReference graphicsState_;
};

class Shader : public META_NS::IntroduceInterfaces<GraphicsState, IShader, IShaderState, META_NS::INamed,
                   META_NS::IPropertyOwner> {
    META_OBJECT(Shader, ClassId::Shader, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(IShader, CullModeFlags, CullMode)
    META_STATIC_PROPERTY_DATA(IShader, SCENE_NS::PolygonMode, PolygonMode)
    META_STATIC_PROPERTY_DATA(IShader, bool, Blend)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_PROPERTY(CullModeFlags, CullMode)
    META_IMPLEMENT_PROPERTY(SCENE_NS::PolygonMode, PolygonMode)
    META_IMPLEMENT_PROPERTY(bool, Blend)

    void OnPropertyChanged(const META_NS::IProperty&) override;

    BASE_NS::string GetName() const override;
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::ShaderResource.Id().ToUid();
    }

    bool SetRenderHandle(RENDER_NS::RenderHandleReference handle) override;
    bool SetShaderState(RENDER_NS::RenderHandleReference shader, RENDER_NS::RenderHandleReference graphics) override;
    bool setShaderStateInProgress_ = false;
};

SCENE_END_NAMESPACE()

#endif