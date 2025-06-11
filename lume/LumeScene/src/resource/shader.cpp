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
#include "shader.h"

#include <algorithm>
#include <mutex>
#include <scene/ext/util.h>

#include <3d/render/default_material_constants.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "../util.h"

SCENE_BEGIN_NAMESPACE()

bool GraphicsState::SetGraphicsState(RENDER_NS::RenderHandleReference handle)
{
    std::unique_lock lock { mutex_ };
    graphicsState_ = handle;
    return true;
}
RENDER_NS::RenderHandleReference GraphicsState::GetGraphicsState() const
{
    std::shared_lock lock { mutex_ };
    return graphicsState_;
}

static bool IsBlendEnabled(const RENDER_NS::GraphicsState& gs)
{
    return std::any_of(gs.colorBlendState.colorAttachments,
        gs.colorBlendState.colorAttachments + gs.colorBlendState.colorAttachmentCount,
        [](const RENDER_NS::GraphicsState::ColorBlendState::Attachment& attachment) { return attachment.enableBlend; });
}

static BASE_NS::string_view GetRenderSlot(const RENDER_NS::GraphicsState& gs)
{
    return IsBlendEnabled(gs) ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                              : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
}

RENDER_NS::GraphicsState GraphicsState::CreateGraphicsState(
    const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference state, BASE_NS::string_view defaultRenderSlot)
{
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    RENDER_NS::GraphicsState gs;
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo;
    if (state) {
        gs = man.GetGraphicsState(state);
        auto id = man.GetRenderSlotId(state);
        vinfo.renderSlot = man.GetRenderSlotName(id);
    } else {
        auto renderSlotId = man.GetRenderSlotId(defaultRenderSlot);
        auto rsd = man.GetRenderSlotData(renderSlotId);
        gs = man.GetGraphicsState(rsd.graphicsState);
        vinfo.renderSlot = GetRenderSlot(gs);
    }
    std::unique_lock lock { mutex_ };
    graphicsState_ =
        man.CreateGraphicsState({ "SceneGS_" + BASE_NS::string(BASE_NS::to_string(uintptr_t(this))), gs }, vinfo);
    return gs;
}

bool GraphicsState::UpdateGraphicsState(
    const IRenderContext::Ptr& context, const RENDER_NS::GraphicsState& gs, BASE_NS::string_view renderSlot = {})
{
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo { renderSlot };
    if (renderSlot.empty()) {
        vinfo.renderSlot = GetRenderSlot(gs);
    }
    auto s = man.CreateGraphicsState({ "SceneGS_" + BASE_NS::string(BASE_NS::to_string(uintptr_t(this))), gs }, vinfo);
    {
        bool update = false;
        {
            std::unique_lock lock { mutex_ };
            update = s.GetHandle() != graphicsState_.GetHandle();
            if (update) {
                graphicsState_ = s;
            }
        }
        if (update) {
            if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
                META_NS::Invoke<META_NS::IOnChanged>(ev);
            }
        }
    }
    return true;
}

RENDER_NS::GraphicsState GraphicsState::CreateNewGraphicsState(const IRenderContext::Ptr& context, bool blend)
{
    // notice: this is wrong for user shaders, fix later
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    auto oldgs = GetGraphicsState(context);
    auto renderSlotId =
        man.GetRenderSlotId(blend ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                                  : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
    auto rsd = man.GetRenderSlotData(renderSlotId);
    auto gs = man.GetGraphicsState(rsd.graphicsState);
    gs.inputAssembly = oldgs.inputAssembly;
    gs.depthStencilState = oldgs.depthStencilState;
    gs.rasterizationState = oldgs.rasterizationState;
    return gs;
}

RENDER_NS::GraphicsState GraphicsState::GetGraphicsState(const IRenderContext::Ptr& context) const
{
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    std::shared_lock lock { mutex_ };
    return man.GetGraphicsState(graphicsState_);
}

BASE_NS::string Shader::GetName() const
{
    return META_NS::GetValue(Name());
}

bool Shader::SetRenderHandle(RENDER_NS::RenderHandleReference handle)
{
    return SetShaderState(handle, {});
}

bool Shader::SetShaderState(RENDER_NS::RenderHandleReference shader, RENDER_NS::RenderHandleReference graphics)
{
    IRenderContext::Ptr context = context_.lock();
    if (!context) {
        CORE_LOG_E("Invalid context");
        return false;
    }
    RENDER_NS::IShaderManager::IdDesc desc;
    RENDER_NS::GraphicsState gs;
    BASE_NS::string renderSlot;
    context
        ->AddTask([&] {
            renderSlot = CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
            if (shader) {
                desc = context->GetRenderer()->GetDevice().GetShaderManager().GetIdDesc(shader);
                renderSlot = desc.renderSlot;
            }
            gs = CreateGraphicsState(context, graphics, renderSlot);
        })
        .Wait();

    {
        std::unique_lock lock { mutex_ };
        handle_ = shader;
    }

    // todo: notify in same thread?
    setShaderStateInProgress_ = true;
    Name()->SetValue(desc.displayName);
    CullMode()->SetValue(static_cast<CullModeFlags>(gs.rasterizationState.cullModeFlags));
    Blend()->SetValue(IsBlendEnabled(gs));
    setShaderStateInProgress_ = false;
    UpdateGraphicsState(context, gs, renderSlot);
    if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
        META_NS::Invoke<META_NS::IOnChanged>(ev);
    }

    return true;
}

void Shader::OnPropertyChanged(const META_NS::IProperty& p)
{
    if (setShaderStateInProgress_) {
        // ignore the property sets during SetShaderState.
        return;
    }
    auto context = context_.lock();
    auto state = GetGraphicsState();
    if (!context || !state) {
        return;
    }
    if (p.GetName() == "CullMode") {
        context
            ->AddTask([&] {
                auto gs = GetGraphicsState(context);
                gs.rasterizationState.cullModeFlags = static_cast<RENDER_NS::CullModeFlags>(CullMode()->GetValue());
                UpdateGraphicsState(context, gs);
            })
            .Wait();
    } else if (p.GetName() == "Blend") {
        context->AddTask([&] { UpdateGraphicsState(context, CreateNewGraphicsState(context, Blend()->GetValue())); })
            .Wait();
    }
}

SCENE_END_NAMESPACE()
