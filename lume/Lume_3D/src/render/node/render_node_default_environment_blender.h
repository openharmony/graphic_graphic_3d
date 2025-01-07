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

#ifndef CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_ENVIRONMENT_BLENDER_H
#define CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_ENVIRONMENT_BLENDER_H

#include <3d/namespace.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

RENDER_BEGIN_NAMESPACE()
struct RenderNodeGraphInputs;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultEnvironmentBlender final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultEnvironmentBlender() = default;
    ~RenderNodeDefaultEnvironmentBlender() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "291720f5-1a00-446f-ab98-cc62f6c29562" };
    static constexpr char const * TYPE_NAME = "RenderNodeDefaultEnvironmentBlender";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct DefaultImagesAndSamplers {
        RENDER_NS::RenderHandle cubemapSamplerHandle;

        RENDER_NS::RenderHandle linearHandle;
        RENDER_NS::RenderHandle nearestHandle;
        RENDER_NS::RenderHandle linearMipHandle;

        RENDER_NS::RenderHandle skyBoxRadianceCubemapHandle;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void InitializeShaderData();
    void InitCreateBinders();
    void ParseRenderNodeInputs();
    void UpdateImageData();
    RENDER_NS::RenderHandle GetEnvironmentTargetHandle(const RenderCamera::Environment& environment);
    void FillBlendEnvironments(BASE_NS::array_view<const RenderCamera::Environment> environments, uint32_t idx,
        BASE_NS::array_view<RENDER_NS::RenderHandle>& blendEnvs) const;
    void UpdateSet0(RENDER_NS::IRenderCommandList& cmdList,
        BASE_NS::array_view<const RENDER_NS::RenderHandle> blendImages, const uint32_t envIdx, const uint32_t mipIdx);
    void ExecuteSingleEnvironment(RENDER_NS::IRenderCommandList& cmdList,
        BASE_NS::array_view<const RenderCamera::Environment> environments, uint32_t idx);

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    // Json resources which might need re-fetching
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;
    };
    JsonInputs jsonInputs_;

    struct BuiltInVariables {
        RENDER_NS::RenderHandle defBlackImage;
        RENDER_NS::RenderHandle defSampler;
    };
    BuiltInVariables builtInVariables_;
    DefaultImagesAndSamplers defaultImagesAndSamplers_;

    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;
    RENDER_NS::RenderNodeHandles::InputResources inputResources_;
    RENDER_NS::RenderHandle shader_;
    RENDER_NS::PipelineLayout pipelineLayout_;
    RENDER_NS::RenderHandle psoHandle_;

    RENDER_NS::RenderHandle environmentUbo_;

    struct AllEnvSets {
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> localSets;
    };
    BASE_NS::vector<AllEnvSets> allEnvSets_;

    SceneRenderDataStores stores_;

    bool valid_ { false };
    bool hasBlendEnvironments_ { false };
    bool hasInitializedShaderData_ { false };

    struct OwnedEnvironmentTargetData {
        uint64_t id { 0ULL };
        RENDER_NS::RenderHandleReference handle;
        bool usedThisFrame { false };
    };
    BASE_NS::vector<OwnedEnvironmentTargetData> envTargetData_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_ENVIRONMENT_BLENDER_H
