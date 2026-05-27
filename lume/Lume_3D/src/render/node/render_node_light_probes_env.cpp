/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "render_node_light_probes_env.h"

#include <algorithm>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/default_constants.h"
#include "render/render_node_scene_util.h"
#include "util/log.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>
}  // namespace
CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr string_view DEFAULT_SKY_SHADER_NAME{"3dshaders://shader/clouds/core3d_dm_env_sky.shader"};
// our light weight straight to screen post processes are only interested in these
static constexpr uint32_t FIXED_CUSTOM_SET3{3U};

struct InputEnvironmentDataHandles {
    RenderHandle cubeHandle;
    RenderHandle cubeBlenderHandle;
    RenderHandle texHandle;
    RenderHandle tlutTexHandle;
    float lodLevel{0.0f};
};

struct EnvLutNaming {
    string_view sceneName;
    string_view cameraName;
    uint64_t cameraId{0U};
};

void ResolveSkyLutHandles(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const EnvLutNaming& naming, InputEnvironmentDataHandles& iedh)
{
    const string fullCameraName = naming.cameraName + '_' + to_hex(naming.cameraId);
    const string skyViewLutName =
        string(naming.sceneName) + DefaultMaterialCameraConstants::CAMERA_SKY_LUT_PREFIX_NAME + fullCameraName;
    iedh.texHandle = gpuResourceMgr.GetImageHandle(skyViewLutName);
    const string transmittanceLutName = string(naming.sceneName) +
                                        DefaultMaterialCameraConstants::CAMERA_TRANSMITTANCE_LUT_PREFIX_NAME +
                                        fullCameraName;
    iedh.tlutTexHandle = gpuResourceMgr.GetImageHandle(transmittanceLutName);
}

InputEnvironmentDataHandles GetEnvironmentDataHandles(const IRenderDataStoreDefaultCamera& dsCamera,
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderNodeDefaultEnv::DefaultImages& defaultImages,
    const RenderCamera& cam, const EnvLutNaming& naming)
{
    InputEnvironmentDataHandles iedh;
    iedh.texHandle = defaultImages.texHandle;
    iedh.cubeHandle = defaultImages.cubeHandle;
    iedh.cubeBlenderHandle = defaultImages.cubeHandle;

    const auto& env = cam.environment;
    const bool dynCubemap = (env.multiEnvCount > 0U);
    if (env.envMap || dynCubemap) {
        const RenderHandle handle = env.envMap.GetHandle();
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
        if ((env.backgroundType == RenderCamera::Environment::BG_TYPE_IMAGE) ||
            (env.backgroundType == RenderCamera::Environment::BG_TYPE_EQUIRECTANGULAR)) {
            if (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D) {
                iedh.texHandle = handle;
            } else {
                PLUGIN_LOG_ONCE_E("inv_env_2d_bg_type", "invalid environment map, type does not match background type");
            }
        } else if (env.backgroundType == RenderCamera::Environment::BG_TYPE_CUBEMAP) {
            bool valid = false;
            if (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE) {
                iedh.cubeHandle = handle;
                valid = true;
            }
            if (dynCubemap && (env.multiEnvCount >= 2U)) {
                PLUGIN_STATIC_ASSERT(DefaultMaterialCameraConstants::MAX_CAMERA_MULTI_ENVIRONMENT_COUNT >= 2U);
                const RenderCamera::Environment env1 = dsCamera.GetEnvironment(env.multiEnvIds[0U]);
                const RenderCamera::Environment env2 = dsCamera.GetEnvironment(env.multiEnvIds[1U]);
                iedh.cubeHandle = env1.envMap.GetHandle();
                iedh.cubeBlenderHandle = env2.envMap.GetHandle();
                valid = true;
            }
            if (!valid) {
                PLUGIN_LOG_ONCE_E("inv_env_cu_bg_type", "invalid environment map, type does not match background type");
            }
        }
        iedh.lodLevel = env.envMapLodLevel;
    }
    if (env.backgroundType == RenderCamera::Environment::BG_TYPE_SKY) {
        ResolveSkyLutHandles(gpuResourceMgr, naming, iedh);
    } else {
        iedh.tlutTexHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE_WHITE");
    }

    return iedh;
}
}  // namespace

void RenderNodeLightProbesEnv::InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    RenderNodeDefaultEnv::ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};
    currentBgType_ = {RenderCamera::Environment::BG_TYPE_NONE};

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    cubemapSampler =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    defaultImages_.texHandle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR);
    defaultImages_.cubeHandle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_SKYBOX_CUBEMAP);
    rngRenderPass_ = renderNodeContextMgr.GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);

    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();

    const auto envBakeShader =
        shaderMgr.GetShaderDataByShaderName("3dshaders://shader/core3d_dm_light_probes_env.shader");

    defaultShaderData_.shader = envBakeShader.shader;
    defaultShaderData_.pl = envBakeShader.pipelineLayout;
    defaultShaderData_.plData = shaderMgr.GetPipelineLayout(defaultShaderData_.pl);
    defaultSkyShader_ = shaderMgr.GetShaderHandle(DEFAULT_SKY_SHADER_NAME);

    RenderNodeDefaultEnv::CreateDescriptorSets();

    shShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/spherical_harmonics_random_sampled.shader");
    shPipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(shShader_);
    shPsoHandle_ = renderNodeContextMgr_->GetPsoManager().GetComputePsoHandle(shShader_, shPipelineLayout_, {});
}

void RenderNodeLightProbesEnv::RenderData(
    const IRenderDataStoreDefaultCamera& dsCamera, IRenderCommandList& cmdList, const uint32_t viewMatrixIndex)
{
    // re-fetch global descriptor sets every frame
    RenderNodeSceneUtil::FrameGlobalDescriptorSets fgds =
        RenderNodeSceneUtil::GetFrameGlobalDescriptorSets(*renderNodeContextMgr_,
            stores_,
            cameraName_,
            RenderNodeSceneUtil::FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_0);
    if (!fgds.valid) {
        return;
    }

    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);
    if (fsrEnabled_) {
        cmdList.SetDynamicStateFragmentShadingRate({1u, 1u},
            FragmentShadingRateCombinerOps{
                CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE, CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE});
    }

    const auto& cam = currentScene_.camData.camera;
    const RenderCamera::Environment& renderEnv = cam.environment;
    // sky or basic default
    const RenderHandle defaultShader = (renderEnv.backgroundType == RenderCamera::Environment::BG_TYPE_SKY)
                                           ? defaultSkyShader_
                                           : defaultShaderData_.shader;
    const RenderHandle shaderHandle = renderEnv.shader ? renderEnv.shader.GetHandle() : defaultShader;
    const RenderHandle stateHandle = renderEnv.graphicsState ? renderEnv.graphicsState.GetHandle() : RenderHandle{};
    // check for pso changes
    if ((renderEnv.backgroundType != currentBgType_) || (currShaderData_.shader.id != shaderHandle.id) ||
        (currShaderData_.state.id != stateHandle.id) ||
        (currentCameraShaderFlags_ != currentScene_.cameraShaderFlags) ||
        (!RenderHandleUtil::IsValid(currShaderData_.pso))) {
        currentBgType_ = cam.environment.backgroundType;
        currentCameraShaderFlags_ = currentScene_.cameraShaderFlags;
        currShaderData_ = GetPso(shaderHandle, stateHandle, currentBgType_, currentRenderPPConfiguration_);
    }

    cmdList.BindPipeline(currShaderData_.pso);
    cmdList.BindDescriptorSet(0U, fgds.set0);

    if ((!currShaderData_.customSet) && builtInSet3_) {
        const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const auto envDataHandles = GetEnvironmentDataHandles(dsCamera,
            gpuResourceMgr,
            defaultImages_,
            currentScene_.camData.camera,
            EnvLutNaming{stores_.dataStoreNameScene, jsonInputs_.customCameraName, jsonInputs_.customCameraId});
        // set 3, bind combined image samplers
        auto& binder = *builtInSet3_;
        {
            uint32_t bindingIndex = 0;
            binder.BindImage(bindingIndex++, envDataHandles.texHandle, cubemapSampler);
            binder.BindImage(bindingIndex++, envDataHandles.cubeHandle, cubemapSampler);
            binder.BindImage(bindingIndex++, envDataHandles.cubeBlenderHandle, cubemapSampler);
            binder.BindImage(bindingIndex++, envDataHandles.tlutTexHandle, cubemapSampler);

            BindableBuffer vmBuffer;
            vmBuffer.byteSize = static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * viewInvMatrices_.size());
            vmBuffer.handle = probeViewInvMatricesUbo_.GetHandle();
            vmBuffer.byteOffset = 0;
            binder.BindBuffer(bindingIndex++, vmBuffer);
        }
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(FIXED_CUSTOM_SET3, binder.GetDescriptorSetHandle());
    }

    // custom set 3 resources
    bool validDraw = true;
    if (currShaderData_.customSet) {
        validDraw = (renderEnv.customResourceHandles[0]) ? true : false;
        validDraw = validDraw && UpdateAndBindCustomSet(cmdList, renderEnv);
    }

    // push constant for probe-face view matrix indexing
    PushConstant pc{ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t)};
    cmdList.PushConstantData(pc, arrayviewU8(viewMatrixIndex));

    if (validDraw) {
        cmdList.Draw(3u, 1u, 0u, 0u);
    }
}

void RenderNodeLightProbesEnv::ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));

    // upload
    uint8_t* const mappedData = reinterpret_cast<uint8_t*>(
        renderNodeContextMgr_->GetGpuResourceManager().MapBufferMemory(probeViewInvMatricesUbo_.GetHandle()));

    if (mappedData) {
        const uint32_t byteSize = static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * viewInvMatrices_.size());
        CloneData(mappedData, byteSize, viewInvMatrices_.data(), byteSize);
        renderNodeContextMgr_->GetGpuResourceManager().UnmapBuffer(probeViewInvMatricesUbo_.GetHandle());
    }

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "3DLightProbeEnv", DefaultDebugConstants::DEFAULT_DEBUG_COLOR);

    const auto& cam = currentScene_.camData.camera;

    uint32_t probesToBake = static_cast<uint32_t>(cam.lightProbeBakingData.perProbeData.size());
    if (probesToBake == 0u) {
        return;
    }
    constexpr uint32_t maxProbesInAtlas = CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION;
    if (probesToBake > maxProbesInAtlas) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RenderNodeLightProbesEnv: Too many probes to bake, max is %d per frame but received %d",
            static_cast<int>(maxProbesInAtlas),
            static_cast<int>(probesToBake));
        probesToBake = maxProbesInAtlas;
#endif
    }

    // set render passes and render
    for (uint32_t probeIdx = 0u; probeIdx < probesToBake; ++probeIdx) {
        for (uint32_t faceIdx = 0u; faceIdx < 6u; ++faceIdx) {
            const auto& renderArea = cam.lightProbeBakingData.perProbeData[probeIdx].probeAtlasRenderAreas[faceIdx];

            renderPass_.renderPassDesc.renderArea.offsetX = renderArea.offsetX;
            renderPass_.renderPassDesc.renderArea.offsetY = renderArea.offsetY;
            renderPass_.renderPassDesc.renderArea.extentWidth = renderArea.extentWidth;
            renderPass_.renderPassDesc.renderArea.extentHeight = renderArea.extentHeight;

            currentScene_.viewportDesc.x = static_cast<float>(renderArea.offsetX);
            currentScene_.viewportDesc.y = static_cast<float>(renderArea.offsetY);
            currentScene_.viewportDesc.width = static_cast<float>(renderArea.extentWidth);
            currentScene_.viewportDesc.height = static_cast<float>(renderArea.extentHeight);

            currentScene_.scissorDesc.offsetX = renderArea.offsetX;
            currentScene_.scissorDesc.offsetY = renderArea.offsetY;
            currentScene_.scissorDesc.extentWidth = renderArea.extentWidth;
            currentScene_.scissorDesc.extentHeight = renderArea.extentHeight;

            cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

            if (dataStoreCamera && cam.environment.backgroundType != RenderCamera::Environment::BG_TYPE_NONE) {
                if (cam.layerMask & cam.environment.layerMask) {
                    UpdatePostProcessConfiguration();
                    uint32_t viewMatrixIndex = probeIdx * 6u + faceIdx;
                    RenderData(*dataStoreCamera, cmdList, viewMatrixIndex);
                }
            }

            cmdList.EndRenderPass();
        }
    }
    {
        // After cubemap faces are rendered, calculate SH coefficients
        // Note, this must run even if environment is not rendered (camera has no background)
        // Could do all probes in one dispatch but probeIndex logic does not make that easy right now
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

        cmdList.BindPipeline(shPsoHandle_);
        RenderHandle cubeAtlas = gpuResourceMgr.GetImageHandle("LightProbeCubemapAtlas");
        RenderHandle SHBuffer = gpuResourceMgr.GetBufferHandle("SHBuffer");

        const auto& descBindings = shPipelineLayout_.descriptorSetLayouts[0].bindings;
        const RenderHandle descSetHandle = descriptorSetMgr.CreateOneFrameDescriptorSet(descBindings);
        IDescriptorSetBinder::Ptr binder = descriptorSetMgr.CreateDescriptorSetBinder(descSetHandle, descBindings);
        {
            BindableImage cubemaps{};
            cubemaps.handle = cubeAtlas;
            binder->BindImage(0, cubemaps);
        }
        {
            BindableSampler sampler{};
            sampler.handle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
            binder->BindSampler(1, sampler);
        }
        {
            BindableBuffer sh;
            sh.handle = SHBuffer;
            binder->BindBuffer(2, sh);
        }
        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());

        const auto descHandles = binder->GetDescriptorSetHandle();
        cmdList.BindDescriptorSet(0, descHandles);

        for (uint32_t atlasIdx = 0; atlasIdx < probesToBake; atlasIdx++) {
            const RenderCamera::LightProbeBakingData::PerProbeData& bakeData =
                cam.lightProbeBakingData.perProbeData[atlasIdx];

            struct PushConstantLocalStruct {
                vec2 sourceUVSize;
                vec2 sourceUVOffset;
                uint32_t probeOutputIndex;
                uint32_t samplesPerThread;
                float maxSampleRadiance;
                uint32_t padding[1];
            };
            static_assert(sizeof(PushConstantLocalStruct) % 16u == 0);

            constexpr uint32_t samplesPerThread = 16u;

            // Clamp input radiance to 10.0 max, same as our offline baker.
            // HDR radiance cubemaps can have direct sunlight in them which probes should
            // not consider (or we could expose this as a 'radiance clamp' setting on probe group).
            // This clamp works as a filter to remove direct sunlight, which can have values
            // up to ~100k.
            constexpr float maxSampleRadiance = 10.0f;

            const auto& firstFaceRenderArea = bakeData.probeAtlasRenderAreas[0];
            vec2 sourceUVSize = vec2{
                static_cast<float>(6u * firstFaceRenderArea.extentWidth) /
                    static_cast<float>(cam.lightProbeBakingData.probeAtlasWidth),
                static_cast<float>(1u * firstFaceRenderArea.extentHeight) /
                    static_cast<float>(cam.lightProbeBakingData.probeAtlasHeight),
            };
            vec2 sourceUVOffset = vec2{
                static_cast<float>(firstFaceRenderArea.offsetX) /
                    static_cast<float>(cam.lightProbeBakingData.probeAtlasWidth),
                static_cast<float>(firstFaceRenderArea.offsetY) /
                    static_cast<float>(cam.lightProbeBakingData.probeAtlasHeight),
            };
            const PushConstantLocalStruct pushData{
                sourceUVSize,
                sourceUVOffset,
                bakeData.probeIndex,
                samplesPerThread,
                maxSampleRadiance,
                {0},
            };
            cmdList.PushConstant(shPipelineLayout_.pushConstant, reinterpret_cast<const uint8_t*>(&pushData));
            cmdList.Dispatch(1, 1, 1);
        }
    }
}

void RenderNodeLightProbesEnv::PreExecuteFrame()
{
    RenderNodeDefaultEnv::PreExecuteFrame();
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));

    if (dataStoreScene && dataStoreCamera) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera);
    }
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));

    const bool validRenderDataStore = dataStoreScene && dataStoreCamera && dataStoreMaterial;
    if (validRenderDataStore) {
        auto& subpassDesc = renderPass_.subpassDesc;
        if ((subpassDesc.depthAttachmentCount == 1) &&
            (subpassDesc.depthAttachmentIndex < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT)) {
            auto& attRef = renderPass_.renderPassDesc.attachments[subpassDesc.depthAttachmentIndex];
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
            attRef.storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        }

        for (uint32_t idx = 0; idx < subpassDesc.colorAttachmentCount; ++idx) {
            if (subpassDesc.colorAttachmentIndices[idx] < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT) {
                auto& attRef = renderPass_.renderPassDesc.attachments[subpassDesc.colorAttachmentIndices[idx]];
                attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
            }
        }
    } else {
        PLUGIN_LOG_E("invalid render data stores in RenderNodeDefaultMaterialRenderSlot");
    }

    static const Math::Vec3 cubeFaceDirections[6] = {
        {1.0f, 0.0f, 0.0f},   // +x
        {-1.0f, 0.0f, 0.0f},  // -x
        {0.0f, 1.0f, 0.0f},   // +y
        {0.0f, -1.0f, 0.0f},  // -y
        {0.0f, 0.0f, 1.0f},   // +z
        {0.0f, 0.0f, -1.0f}   // -z
    };

    static const Math::Vec3 cubeFaceUps[6] = {
        {0.0f, -1.0f, 0.0f},  // +x
        {0.0f, -1.0f, 0.0f},  // -x
        {0.0f, 0.0f, 1.0f},   // +y
        {0.0f, 0.0f, -1.0f},  // -y
        {0.0f, -1.0f, 0.0f},  // +z
        {0.0f, -1.0f, 0.0f}   // -z
    };

    RenderCamera& cam = currentScene_.camData.camera;

    viewInvMatrices_.clear();

    // collect view matrices
    for (uint32_t probeIdx = 0; probeIdx < cam.lightProbeBakingData.perProbeData.size(); probeIdx++) {
        const RenderCamera::LightProbeBakingData::PerProbeData& bakeData =
            cam.lightProbeBakingData.perProbeData[probeIdx];

        for (size_t face = 0; face < 6u; face++) {
            // view matrix for this cubemap face
            const Math::Vec3 target = bakeData.probePosition + cubeFaceDirections[face];
            viewInvMatrices_.push_back(
                Math::Inverse(Math::LookAtRh(bakeData.probePosition, target, cubeFaceUps[face])));
        }
    }

    // upload to GPU and update UBO
    if (!probeViewInvMatricesUbo_) {
        GpuBufferDesc desc;
        desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                   MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                   EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;

        desc.byteSize = static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * viewInvMatrices_.size());
        probeViewInvMatricesUbo_ = renderNodeContextMgr_->GetGpuResourceManager().Create(desc);
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeLightProbesEnv::Create()
{
    return new RenderNodeLightProbesEnv();
}

void RenderNodeLightProbesEnv::Destroy(RENDER_NS::IRenderNode* instance)
{
    delete static_cast<RenderNodeLightProbesEnv*>(instance);
}

CORE3D_END_NAMESPACE()
