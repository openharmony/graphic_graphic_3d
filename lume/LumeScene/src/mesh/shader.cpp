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
    if (handle.GetHandleType() != RENDER_NS::RenderHandleType::COMPUTE_SHADER_STATE_OBJECT &&
        handle.GetHandleType() != RENDER_NS::RenderHandleType::SHADER_STATE_OBJECT) {
        CORE_LOG_E("Invalid type for shader handle");
        return false;
    }
    {
        std::shared_lock lock { mutex_ };
        if (handle.GetHandle() == handle_.GetHandle() && entity_ == ent && graphicsState_) {
            return true;
        }
    }

    CORE_NS::EntityReference gstate;
    RENDER_NS::IShaderManager::IdDesc desc;
    RENDER_NS::RenderHandleReference ghandle;
    scene
        ->AddTask([&] {
            if (!ent) {
                ent = HandleFromRenderResource(scene, handle);
            }
            auto& shaderManager = scene->GetRenderContext().GetDevice().GetShaderManager();
            if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                    scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                ghandle = shaderManager.GetGraphicsStateHandleByShaderHandle(handle);
                if (ghandle) {
                    auto& entityManager = scene->GetEcsContext().GetNativeEcs()->GetEntityManager();
                    gstate = CORE3D_NS::GetOrCreateEntityReference(entityManager, *rhman, ghandle);
                }
            }
            desc = shaderManager.GetIdDesc(handle);
        })
        .Wait();

    if (!ent) {
        return false;
    }
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
CORE_NS::EntityReference Shader::GetEntity() const
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
                SetRenderHandle(inter, handle, {});
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

void Shader::UpdateGraphicsState(
    const IInternalScene::Ptr& scene, RENDER_NS::RenderHandleReference h, const RENDER_NS::GraphicsState& gs)
{
    auto& man = scene->GetRenderContext().GetDevice().GetShaderManager();
    RENDER_NS::IShaderManager::GraphicsStateVariantCreateInfo vinfo {};
    vinfo.variant = BASE_NS::string(BASE_NS::to_string(intptr_t(this)));
    auto handle = man.CreateGraphicsState({ Uri()->GetValue(), gs }, vinfo);
    if (h.GetHandle() != handle.GetHandle()) {
        std::shared_lock lock { mutex_ };
        if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
            if (auto m = rhman->Write(graphicsState_)) {
                m->reference = handle;
            }
        }
        graphicsStateHandle_ = handle;
    }
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
                UpdateGraphicsState(inter, h, gs);
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
                auto& man = inter->GetRenderContext().GetDevice().GetShaderManager();
                auto gs = man.GetGraphicsState(h);
                if (gs.colorBlendState.colorAttachmentCount == 0) {
                    gs.colorBlendState.colorAttachmentCount = 1;
                }
                gs.colorBlendState.colorAttachments[0].enableBlend = enableBlend;
                UpdateGraphicsState(inter, h, gs);
            }
            return static_cast<bool>(h);
        });
    }
    return {};
}

SCENE_END_NAMESPACE()
