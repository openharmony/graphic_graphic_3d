/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <core/log.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_node_system.h>

#include "mrt_system.h"

MRT_BEGIN_NAMESPACE()

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

MRTSystem::MRTSystem(IEcs& ecs) : ecs_(ecs)
{
    IClassRegister* cr = ecs.GetClassFactory().GetInterface<IClassRegister>();
    if (!cr) {
        CORE_LOG_E("MRTSystem init failed: classRegister is nullptr");
        return;
    }
    renderContext_ = GetInstance<IRenderContext>(*cr, UID_RENDER_CONTEXT);
}

MRTSystem::~MRTSystem()
{}

string_view MRTSystem::GetName() const
{
    return "MRTSystem";
}

Uid MRTSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* MRTSystem::GetProperties()
{
    return propertyApi_.GetData();
}

const IPropertyHandle* MRTSystem::GetProperties() const
{
    return propertyApi_.GetData();
}

void MRTSystem::SetProperties(const IPropertyHandle&)
{
    return;
}

bool MRTSystem::IsActive() const
{
    return active_;
}

void MRTSystem::SetActive(bool state)
{
    active_ = state;
}

void MRTSystem::Initialize()
{
    CORE_LOG_E("MRTSystem::Initialize()");
}

void MRTSystem::Uninitialize()
{
    CORE_LOG_E("MRTSystem::Uninitialize()");
}

void MRTSystem::UpdateShaders(float zNear, float zFar)
{
    auto cameraManager = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecs_);

    // Apply shaders to all mesh
    {
        auto* meshMgr = GetManager<IMeshComponentManager>(ecs_);
        auto& shaderMgr = renderContext_->GetDevice().GetShaderManager();

        auto createGfxState = [&shaderMgr, this](const string_view uri, const string_view variantName) {
            EntityReference gfxState = ecs_.GetEntityManager().CreateReferenceCounted();
            auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs_);
            renderHandleManager->Create(gfxState);
            renderHandleManager->Write(gfxState)->reference = shaderMgr.GetGraphicsStateHandle(uri, variantName);
            return gfxState;
        };

        for (IComponentManager::ComponentId id = 0; id < meshMgr->GetComponentCount(); ++id) {
            const auto entity = meshMgr->GetEntity(id);
            auto meshHandle = meshMgr->Read(entity);
            if (!meshHandle) {
                continue;
            }
            for (auto submesh : meshHandle->submeshes) {
                auto materialHandle = GetManager<IMaterialComponentManager>(ecs_)->Write(submesh.material);
                if (!materialHandle) {
                    continue;
                }
                materialHandle->materialShader.shader = GetOrCreateEntityReference(
                    ecs_, shaderMgr.GetShaderHandle("mrt_rofs://shaders/shader/mrt_dm_fw.shader"));

                // use specular slot to pass the nearPlane and far Plane into the shader
                materialHandle->textures[MaterialComponent::TextureIndex::SPECULAR].factor = {zNear, zFar, 1.0f, 1.0f};
                materialHandle->materialShader.graphicsState =
                    createGfxState("mrtshaderstates://mrt_mpi_dm_fw.shadergs", "TRANSLUCENT_FW_DS");
            }
        }
    }

    // TODO: Check whether this is required?
    // Apply shaders to environment
    {
        IEnvironmentComponentManager* envManager = GetManager<IEnvironmentComponentManager>(ecs_);
        const auto environments = static_cast<IComponentManager::ComponentId>(envManager->GetComponentCount());
        auto& shaderMgr = renderContext_->GetDevice().GetShaderManager();
        auto cameraEntity = cameraManager->GetEntity(0);
        auto cameraHandle = cameraManager->Write(cameraEntity);

        if (auto envDataHandle = envManager->Write(cameraHandle->environment); envDataHandle) {
            EnvironmentComponent& envComponent = *envDataHandle;
            envComponent.shader = GetOrCreateEntityReference(
                ecs_, shaderMgr.GetShaderHandle("mrt_rofs://shaders/shader/mrt_dm_env.shader"));
        }
    }
}

bool MRTSystem::Update(bool frameRenderingQueued, uint64_t time, uint64_t delta)
{
    float zNear = 0.0f;
    float zFar = 0.0f;
    if (!inited_) {
        // replace rng
        auto cameraManager = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecs_);
        if (cameraManager->GetComponentCount() > 0) {
            auto cameraEntity = cameraManager->GetEntity(0);
            if (auto cameraHandle = cameraManager->Write(cameraEntity); cameraHandle) {
                CORE3D_NS::CameraComponent& cameraComponent = *cameraHandle;
                cameraComponent.customRenderNodeGraphFile = "mrt_rofs://rendernodegraphs/mrt_rng_cam_scene_lwrp.rng";
                zNear = cameraComponent.zNear;
                zFar = cameraComponent.zFar;
            }
        }
        UpdateShaders(zNear, zFar);
        inited_ = true;
    }
    return true;
}

const IEcs& MRTSystem::GetECS() const
{
    return ecs_;
}

CORE_NS::ISystem* MRTSystemInstance(IEcs& ecs)
{
    return new MRTSystem(ecs);
}

void MRTSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<MRTSystem*>(instance);
}

MRT_END_NAMESPACE()