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

static RENDER_NS::RenderHandleReference DoCreateGraphicsState(RENDER_NS::IShaderManager& man,
    const RENDER_NS::GraphicsState& gs, const RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo& vinfo,
    uint32_t slotId)
{
    // use graphics state hash for the path, so that same states get reused. Shader manager always keeps reference
    // to the state and so the states are not automatically freed, causing them to pile up if always creating a new one.
    auto hash = man.HashGraphicsState(gs, slotId);

    if (auto existing = man.GetGraphicsStateHandleByHash(hash); existing.GetHandle().id
        != RENDER_NS::INVALID_RESOURCE_HANDLE){
        return existing;
    }

    auto hashstr = BASE_NS::string(BASE_NS::to_string(hash));
    auto handle = man.CreateGraphicsState({"SceneGS_" + hashstr, gs}, vinfo);

    return handle;
}

RENDER_NS::GraphicsState GraphicsState::CreateGraphicsState(
    const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference state, BASE_NS::string_view defaultRenderSlot)
{
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    RENDER_NS::GraphicsState gs;
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo;
    BASE_NS::string slot;
    uint32_t renderSlotId{};
    if (state) {
        gs = man.GetGraphicsState(state);
        renderSlotId = man.GetRenderSlotId(state);
        slot = man.GetRenderSlotName(renderSlotId);
    } else {
        if (defaultRenderSlot.empty()) {
            defaultRenderSlot = CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
        }
        renderSlotId = man.GetRenderSlotId(defaultRenderSlot);
        auto rsd = man.GetRenderSlotData(renderSlotId);
        gs = man.GetGraphicsState(rsd.graphicsState);
        slot = GetRenderSlot(gs);
    }
    if (slot.empty()) {
        slot = defaultRenderSlot;
    }
    if (slot.empty()) {
        // no slot info, create from default opaque.
        slot = CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
        renderSlotId = man.GetRenderSlotId(slot);
        auto rsd = man.GetRenderSlotData(renderSlotId);
        gs = man.GetGraphicsState(rsd.graphicsState);
    }
    vinfo.renderSlot = slot;
    std::unique_lock lock { mutex_ };
    graphicsState_ = DoCreateGraphicsState(man, gs, vinfo, renderSlotId);
    return gs;
}

bool GraphicsState::UpdateGraphicsState(
    const IRenderContext::Ptr& context, const RENDER_NS::GraphicsState& gs,
    bool blend, BASE_NS::string_view renderSlot = {})
{
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo { renderSlot };
    if (renderSlot.empty()) {
        vinfo.renderSlot = GetRenderSlot(gs);
    }
    RENDER_NS::RenderHandleReference s;
    auto renderSlotId =
        man.GetRenderSlotId(blend ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
            : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);

    bool update = false;
    {
        std::unique_lock lock { mutex_ };
        if (depthOptions_.has_value()) {
            // Override options for depth have been set, override these from the default. Note that Blend still controls the
            // render slot id
            auto gss = gs;
            gss.depthStencilState.enableDepthTest = depthOptions_->enableDepthTest;
            gss.depthStencilState.enableDepthWrite = depthOptions_->enableDepthWrite;
            s = DoCreateGraphicsState(man, gss, vinfo, renderSlotId);
        } else {
            s = DoCreateGraphicsState(man, gs, vinfo, renderSlotId);
        }
        {
            update = s.GetHandle() != graphicsState_.GetHandle();
            if (update) {
                graphicsState_ = s;
            }
        }
    }
    if (update) {
        if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
            META_NS::Invoke<META_NS::IOnChanged>(ev);
        }
    }

    return true;
}

RENDER_NS::GraphicsState GraphicsState::CreateNewGraphicsState(const IRenderContext::Ptr& context, bool blend)
{
    // notice: this is wrong for user shaders, fix later
    // this uses the default graphicstate for either translucent or opaque slots. (ie. possibly wrong for user shaders
    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
    auto renderSlotId =
        man.GetRenderSlotId(blend ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                                  : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
    auto rsd = man.GetRenderSlotData(renderSlotId);
    return man.GetGraphicsState(rsd.graphicsState);
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
    context->RunDirectlyOrInTask([&] {
        BASE_NS::string_view renderSlot;
        if (shader) {
            desc = context->GetRenderer()->GetDevice().GetShaderManager().GetIdDesc(shader);
            if (!desc.renderSlot.empty()) {
                renderSlot = desc.renderSlot;
            }
        }
        gs = CreateGraphicsState(context, graphics, renderSlot);
    });

    {
        std::unique_lock lock { mutex_ };
        handle_ = shader;
    }

    // todo: notify in same thread?
    Name()->SetValue(desc.displayName);
    CullMode()->SetValue(static_cast<CullModeFlags>(gs.rasterizationState.cullModeFlags));
    PolygonMode()->SetValue(static_cast<SCENE_NS::PolygonMode>(gs.rasterizationState.polygonMode));
    Blend()->SetValue(IsBlendEnabled(gs));

    if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
        META_NS::Invoke<META_NS::IOnChanged>(ev);
    }

    return true;
}

void Shader::SetShaderStateOverride(const ColorBlendOptions* colorOptions, const DepthStencilOptions* depthOptions,
    const RasterizationOptions* /*rasterizationOptions*/)
{
    depthOptions_.reset();
    colorOptions_.reset();
    if (depthOptions) {
        depthOptions_ = *depthOptions;
    }
    if (colorOptions) {
        colorOptions_ = *colorOptions;
    }
    // Update graphics state always when SetShaderStateOverride is called, assume that there is some change
    auto context = context_.lock();
    auto state = GetGraphicsState();
    if (context && state) {
	    auto blend = GetBlend();
        context->RunDirectlyOrInTask(
            [&] { UpdateGraphicsState(context, CreateNewGraphicsState(context, blend), blend); });
    }
}

bool Shader::GetBlend() const
{
    // If colorOptions_ is set (through SetShaderStateOverride()) then always use that value ignoring Blend property.
    return colorOptions_.has_value() ? colorOptions_->enableBlend : META_NS::GetValue(Blend());
}
void Shader::OnPropertyChanged(const META_NS::IProperty& p)
{
    auto context = context_.lock();
    auto state = GetGraphicsState();
    if (!context || !state) {
        return;
    }
    if (p.GetName() == "CullMode") {
        auto cull = CullMode()->GetValue();
        auto gs = GetGraphicsState(context);
        if (gs.rasterizationState.cullModeFlags != static_cast<RENDER_NS::CullModeFlags>(cull)) {
            gs.rasterizationState.cullModeFlags = static_cast<RENDER_NS::CullModeFlags>(cull);
            context->RunDirectlyOrInTask([&] { UpdateGraphicsState(context, gs, GetBlend()); });
        }
    } else if (p.GetName() == "Blend") {
        auto blend = GetBlend();
        if (IsBlendEnabled(GetGraphicsState(context)) != blend) {
            context->RunDirectlyOrInTask([&] { UpdateGraphicsState(context, CreateNewGraphicsState(context, blend), blend); });
        }
    } else if (p.GetName() == "PolygonMode") {
        context->RunDirectlyOrInTask([&] {
            auto gs = GetGraphicsState(context);
            gs.rasterizationState.polygonMode = static_cast<RENDER_NS::PolygonMode>(PolygonMode()->GetValue());
            UpdateGraphicsState(context, gs, GetBlend());
        });
    }
}

SCENE_END_NAMESPACE()
