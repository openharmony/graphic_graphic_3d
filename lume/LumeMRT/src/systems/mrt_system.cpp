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
#include <render/datastore/intf_render_data_store_pod.h>

#include <cmath>

#include "mrt_system.h"

MRT_BEGIN_NAMESPACE()

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
constexpr string_view POD_DATA_STORE_NAME{"RenderDataStorePod"};
constexpr string_view MRT_RENDER_DATA_STORE_TYPE{"MRTRenderDataStorePod"};
constexpr string_view VERTEX_INPUT_DECLARATIONS{"VertexInputDeclarations"};
}  // namespace

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
    if (renderContext_) {
        auto dataStore = refcnt_ptr<IRenderDataStorePod>(
            renderContext_->GetRenderDataStoreManager().GetRenderDataStore(POD_DATA_STORE_NAME));
        if (dataStore) {
            auto pod = refcnt_ptr<IRenderDataStorePod>(dataStore.get());
            BASE_NS::string_view vertexInputDeclarations =
                "mrtvertexinputdeclarations://mrt_high_precision_uv.shadervid";
            const array_view<const uint8_t> dataView = {
                reinterpret_cast<const uint8_t*>(vertexInputDeclarations.data()), vertexInputDeclarations.length()};
            pod->CreatePod(MRT_RENDER_DATA_STORE_TYPE, VERTEX_INPUT_DECLARATIONS, dataView);
        } else {
            CORE_LOG_E("MRTSystem::Initialize get null data store");
        }
    }
    CORE_LOG_I("MRTSystem::Initialize()");
}

void MRTSystem::Uninitialize()
{
    if (renderContext_) {
        auto dataStore = refcnt_ptr<IRenderDataStorePod>(
            renderContext_->GetRenderDataStoreManager().GetRenderDataStore(POD_DATA_STORE_NAME));
        if (dataStore) {
            auto pod = refcnt_ptr<IRenderDataStorePod>(dataStore.get());
            pod->DestroyPod(MRT_RENDER_DATA_STORE_TYPE, VERTEX_INPUT_DECLARATIONS);
        }
    }
    CORE_LOG_I("MRTSystem::Uninitialize()");
}

void MRTSystem::UpdateShaders(float zNear, float zFar, bool updateNearFarOnly)
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

                // assume the recontructed gltf file does not have any specular material
                // then use specular slot to pass the nearPlane and far Plane into the shader
                materialHandle->textures[MaterialComponent::TextureIndex::SPECULAR].factor = {zNear, zFar, 1.0f, 1.0f};

                if (!updateNearFarOnly) {
                    materialHandle->materialShader.shader = GetOrCreateEntityReference(
                        ecs_, shaderMgr.GetShaderHandle("mrt_rofs://shaders/shader/mrt_dm_fw.shader"));
                    materialHandle->materialShader.graphicsState =
                        createGfxState("mrtshaderstates://mrt_mpi_dm_fw.shadergs", "TRANSLUCENT_FW_DS");
                    // get sampler of base color, i.e. the "real" material of gltf
                    auto samplerEntity = ecs_.GetEntityManager().CreateReferenceCounted();
                    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs_);
                    renderHandleManager->Create(samplerEntity);
                    renderHandleManager->Write(samplerEntity)->reference =
                        renderContext_->GetDevice().GetGpuResourceManager().GetSamplerHandle(
                            "CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");

                    materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].sampler = samplerEntity;
                }
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

        if (!cameraHandle) {
            // avoid crash, may let the buffer to be black
            CORE_LOG_E("MRTSystem:: Apply shaders to environment cameraHandle is NULL");
        } else if (auto envDataHandle = envManager->Write(cameraHandle->environment); envDataHandle) {
            EnvironmentComponent& envComponent = *envDataHandle;
            envComponent.shader = GetOrCreateEntityReference(
                ecs_, shaderMgr.GetShaderHandle("mrt_rofs://shaders/shader/mrt_dm_env.shader"));
        }
    }

    inited_ = true;
}

bool MRTSystem::Update(bool frameRenderingQueued, uint64_t time, uint64_t delta)
{
    float zNear = 0.0f;
    float zFar = 0.0f;
    bool updateNearFar = false;
    // replace rng
    auto cameraManager = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecs_);
    if (cameraManager->GetComponentCount() > 0) {
        auto cameraEntity = cameraManager->GetEntity(0);
        if (auto cameraHandle = cameraManager->Write(cameraEntity); cameraHandle) {
            CORE3D_NS::CameraComponent& cameraComponent = *cameraHandle;
            cameraComponent.customRenderNodeGraphFile = "mrt_rofs://rendernodegraphs/mrt_rng_cam_scene_lwrp.rng";
            updateNearFar =
                fabs(cameraComponent.zNear - zNear_) > 0.001f || fabs(cameraComponent.zFar - zFar_) > 0.001f;
            zNear = cameraComponent.zNear;
            zFar = cameraComponent.zFar;
        }
    }

    if (!inited_ || updateNearFar) {
        UpdateShaders(zNear, zFar, inited_ && updateNearFar);  // init and updateNearFar, then update near far only
        zNear_ = zNear;
        zFar_ = zFar;
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