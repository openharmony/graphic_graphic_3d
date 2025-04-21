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
#include <3d/render/default_material_constants.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/make_callback.h>

#include "util.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string Shader::GetName() const
{
    return Name()->GetValue();
}

bool Shader::SetRenderHandleImpl(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle)
{
    handle_ = handle;
    return true;
}

bool Shader::SetRenderHandle(
    const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle, CORE_NS::EntityReference ent)
{
    return SetShaderState(scene, handle, ent, {}, {});
}

bool Shader::SetShaderState(const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference handle,
    CORE_NS::EntityReference ent, RENDER_NS::RenderHandleReference ghandle, CORE_NS::EntityReference gstate)
{
    {
        std::shared_lock lock { mutex_ };
        if (handle_.GetHandle() == handle.GetHandle() && entity_ == ent && graphicsState_ == gstate &&
            graphicsStateHandle_.GetHandle() == ghandle.GetHandle()) {
            return true;
        }
    }

    RENDER_NS::IShaderManager::IdDesc desc;
    scene
        ->AddTask([&] {
            auto& shaderMgr = scene->GetRenderContext().GetDevice().GetShaderManager();

            if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                    scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                auto renderSlotId =
                    shaderMgr.GetRenderSlotId(CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
                auto rsd = shaderMgr.GetRenderSlotData(renderSlotId);

                if (handle) {
                    if (!ent) {
                        ent = HandleFromRenderResource(scene, handle);
                    }
                    desc = shaderMgr.GetIdDesc(handle);
                } else {
                    desc = shaderMgr.GetIdDesc(rsd.shader);
                }
                if (!ghandle) {
                    ghandle = UpdateGraphicsState(scene, shaderMgr.GetGraphicsState(rsd.graphicsState));
                }
                if (!gstate) {
                    gstate = HandleFromRenderResource(scene, ghandle);
                }
                if (auto m = rhman->Write(gstate)) {
                    m->reference = ghandle;
                }
            }
        })
        .Wait();

    {
        std::unique_lock lock { mutex_ };
        handle_ = handle;
        entity_ = ent;
        graphicsState_ = gstate;
        graphicsStateHandle_ = ghandle;
        scene_ = scene;
    }
    META_ACCESS_PROPERTY(Uri)->SetValue(desc.path);
    META_ACCESS_PROPERTY(Name)->SetValue(desc.displayName);
    META_NS::Invoke<META_NS::IOnChanged>(EventOnResourceChanged(META_NS::MetadataQuery::EXISTING));
    return true;
}
RENDER_NS::RenderHandleReference Shader::GetRenderHandle() const
{
    std::shared_lock lock { mutex_ };
    return handle_;
}
CORE_NS::Entity Shader::GetEntity() const
{
    std::shared_lock lock { mutex_ };
    return entity_;
}

bool Shader::SetGraphicsState(CORE_NS::EntityReference gstate)
{
    std::unique_lock lock { mutex_ };
    graphicsState_ = gstate;
    return true;
}
CORE_NS::EntityReference Shader::GetGraphicsState() const
{
    std::shared_lock lock { mutex_ };
    return graphicsState_;
}

Future<bool> Shader::LoadShader(const IScene::Ptr& s, BASE_NS::string_view uri)
{
    if (auto inter = s->GetInternalScene()) {
        return inter->AddTask([=, path = BASE_NS::string(uri)] {
            auto& man = inter->GetRenderContext().GetDevice().GetShaderManager();
            man.LoadShaderFile(path);
            auto handle = man.GetShaderHandle(path);
            if (handle) {
                // t: notify in correct thread
                SetShaderState(inter, handle, {}, man.GetGraphicsStateHandleByShaderHandle(handle), {});
            }
            return static_cast<bool>(handle);
        });
    }
    return {};
}

IInternalScene::Ptr Shader::GetScene() const
{
    std::shared_lock lock { mutex_ };
    return scene_.lock();
}

RENDER_NS::RenderHandleReference Shader::GetGraphicsStateHandle() const
{
    std::shared_lock lock { mutex_ };
    return graphicsStateHandle_;
}

static BASE_NS::string_view GetRenderSlot(const RENDER_NS::GraphicsState& gs)
{
    return std::any_of(gs.colorBlendState.colorAttachments,
        gs.colorBlendState.colorAttachments + gs.colorBlendState.colorAttachmentCount,
        [](const RENDER_NS::GraphicsState::ColorBlendState::Attachment& attachment) {
            return attachment.enableBlend;
        })
        ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
        : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
}

RENDER_NS::RenderHandleReference Shader::UpdateGraphicsState(
    const IInternalScene::Ptr& scene, const RENDER_NS::GraphicsState& gs)
{
    auto& man = scene->GetRenderContext().GetDevice().GetShaderManager();
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo { GetRenderSlot(gs) };
    auto handle = man.CreateGraphicsState({ BASE_NS::string(BASE_NS::to_string(intptr_t(this))), gs }, vinfo);
    std::unique_lock lock { mutex_ };
    if (graphicsStateHandle_.GetHandle() != handle.GetHandle()) {
        if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
            graphicsStateHandle_ = handle;
            if (auto m = rhman->Write(graphicsState_)) {
                m->reference = graphicsStateHandle_;
            }
        }
    }
    return graphicsStateHandle_;
}

Future<CullModeFlags> Shader::GetCullMode() const
{
    if (auto inter = GetScene()) {
        return inter->AddTask([=] {
            if (auto h = GetGraphicsStateHandle()) {
                auto& man = inter->GetRenderContext().GetDevice().GetShaderManager();
                auto gs = man.GetGraphicsState(h);
                return static_cast<CullModeFlags>(gs.rasterizationState.cullModeFlags);
            }
            return CullModeFlags {};
        });
    }
    return {};
}

Future<bool> Shader::SetCullMode(CullModeFlags cullMode)
{
    if (auto inter = GetScene()) {
        return inter->AddTask([=] {
            auto h = GetGraphicsStateHandle();
            if (h) {
                auto& man = inter->GetRenderContext().GetDevice().GetShaderManager();
                auto gs = man.GetGraphicsState(h);
                gs.rasterizationState.cullModeFlags = static_cast<RENDER_NS::CullModeFlags>(cullMode);
                UpdateGraphicsState(inter, gs);
            }
            return static_cast<bool>(h);
        });
    }
    return {};
}

Future<bool> Shader::IsBlendEnabled() const
{
    if (auto inter = GetScene()) {
        return inter->AddTask([=] {
            if (auto h = GetGraphicsStateHandle()) {
                auto& man = inter->GetRenderContext().GetDevice().GetShaderManager();
                auto gs = man.GetGraphicsState(h);
                if (gs.colorBlendState.colorAttachmentCount) {
                    return static_cast<bool>(gs.colorBlendState.colorAttachments[0].enableBlend);
                }
            }
            return bool {};
        });
    }
    return {};
}

Future<bool> Shader::EnableBlend(bool enableBlend)
{
    if (auto inter = GetScene()) {
        return inter->AddTask([=] {
            auto h = GetGraphicsStateHandle();
            if (h) {
                auto& shaderMgr = inter->GetRenderContext().GetDevice().GetShaderManager();
                auto oldgs = shaderMgr.GetGraphicsState(h);
                auto renderSlotId = shaderMgr.GetRenderSlotId(
                    enableBlend ? CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                                : CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
                auto rsd = shaderMgr.GetRenderSlotData(renderSlotId);

                auto gs = shaderMgr.GetGraphicsState(rsd.graphicsState);
                gs.rasterizationState.cullModeFlags = oldgs.rasterizationState.cullModeFlags;

                if (gs.colorBlendState.colorAttachmentCount == 0) {
                    gs.colorBlendState.colorAttachmentCount = 1;
                }
                gs.colorBlendState.colorAttachments[0].enableBlend = enableBlend;

                UpdateGraphicsState(inter, gs);
            }
            return static_cast<bool>(h);
        });
    }
    return {};
}

SCENE_END_NAMESPACE()
