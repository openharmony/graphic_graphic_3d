/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef RENDER_NODECONTEXT_RENDER_NODE_POST_PROCESS_UTIL_H
#define RENDER_NODECONTEXT_RENDER_NODE_POST_PROCESS_UTIL_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node_post_process_util.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/render_data_structures.h>

#include "nodecontext/render_node_copy_util.h"

RENDER_BEGIN_NAMESPACE()
struct RenderableList;
struct PipelineLayout;
class IRenderNodeContextManager;
class IRenderNodeCommandList;

class RenderNodePostProcessUtil final {
public:
    RenderNodePostProcessUtil() = default;
    ~RenderNodePostProcessUtil() = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr,
        const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo);
    void PreExecute(const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo);
    void Execute(IRenderCommandList& cmdList);

private:
    IRenderNodeContextManager* renderNodeContextMgr_;

    void ParseRenderNodeInputs();
    void UpdateImageData();

    void ProcessPostProcessConfiguration();
    void InitCreateShaderResources();
    void InitCreateBinders();
    void CreatePostProcessInterfaces();

    struct InputOutput {
        BindableImage input;
        BindableImage output;
    };
    void ExecuteBlit(IRenderCommandList& cmdList, const InputOutput& inOut);

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputResources resources;
        RenderNodeGraphInputs::RenderDataStore renderDataStore;
        bool hasChangeableResourceHandles { false };

        uint32_t colorIndex { ~0u };
        uint32_t depthIndex { ~0u };
        uint32_t velocityIndex { ~0u };
        uint32_t historyIndex { ~0u };
        uint32_t historyNextIndex { ~0u };
    };
    JsonInputs jsonInputs_;
    RenderNodeHandles::InputResources inputResources_;

    struct EffectData {
        IShaderManager::ShaderData sd;
        RenderHandle pso;
    };
    EffectData copyData_;

    // additional layer copy
    RenderNodeCopyUtil renderCopyLayer_;

    struct PostProcessInterfaces {
        IRenderPostProcess::Ptr postProcess;
        IRenderPostProcessNode::Ptr postProcessNode;
    };
    PostProcessInterfaces ppLensFlareInterface_;
    PostProcessInterfaces ppRenderBlurInterface_;
    PostProcessInterfaces ppRenderTaaInterface_;
    PostProcessInterfaces ppRenderFxaaInterface_;
    PostProcessInterfaces ppRenderDofInterface_;
    PostProcessInterfaces ppRenderMotionBlurInterface_;
    PostProcessInterfaces ppRenderBloomInterface_;
    PostProcessInterfaces ppRenderUpscaleInterface_;
    PostProcessInterfaces ppRenderCombinedInterface_;

    struct AdditionalImageData {
        RenderHandle black;
        RenderHandle white;
    };
    IRenderNodePostProcessUtil::ImageData images_;
    AdditionalImageData aimg_;

    IRenderNodePostProcessUtil::PostProcessInfo postProcessInfo_;

    BASE_NS::Math::UVec2 outputSize_ { 0U, 0U };

    bool validInputs_ { true };
    bool validInputsForTaa_ { false };
    bool validInputsForDof_ { false };
    bool validInputsForMb_ { false };
    bool validInputsForUpscale_ { false };

    BASE_NS::vector<InputOutput> framePostProcessInOut_;

    struct DefaultSamplers {
        RenderHandle nearest;
        RenderHandle linear;
        RenderHandle mipLinear;
    };
    DefaultSamplers samplers_;
    PostProcessConfiguration ppConfig_;

    struct TemporaryImages {
        uint32_t idx = 0U;
        uint32_t imageCount = 0U;
        uint32_t mipIdx = 0U;
        uint32_t mipImageCount = 0U;
        RenderHandleReference images[2U];
        RenderHandleReference mipImages[2U];
        BASE_NS::Math::Vec4 targetSize { 0.0f, 0.0f, 0.0f, 0.0f };
        uint32_t mipCount[2U];

        RenderHandleReference layerCopyImage;
    };
    TemporaryImages ti_;
    BindableImage GetIntermediateImage(const RenderHandle& input)
    {
        if (ti_.imageCount == 0U) {
            return {};
        }
        if (input == ti_.images[ti_.idx].GetHandle()) {
            ti_.idx = (ti_.idx + 1) % static_cast<uint32_t>(ti_.imageCount);
        }
        return { ti_.images[ti_.idx].GetHandle() };
    }
    BindableImage GetMipImage(const RenderHandle& input)
    {
        if (ti_.mipImageCount == 0U) {
            return {};
        }
        if (input == ti_.mipImages[ti_.mipIdx].GetHandle()) {
            ti_.mipIdx = (ti_.mipIdx + 1) % static_cast<uint32_t>(ti_.mipImageCount);
        }
        return { ti_.mipImages[ti_.mipIdx].GetHandle() };
    }

    struct AllBinders {
        IDescriptorSetBinder::Ptr copyBinder;
    };
    AllBinders binders_;

    DeviceBackendType deviceBackendType_ { DeviceBackendType::VULKAN };

    bool glOptimizedLayerCopyEnabled_ { false };
};

class RenderNodePostProcessUtilImpl final : public IRenderNodePostProcessUtil {
public:
    RenderNodePostProcessUtilImpl() = default;
    ~RenderNodePostProcessUtilImpl() override = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr,
        const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo) override;
    void PreExecute(const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo) override;
    void Execute(IRenderCommandList& cmdList) override;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

private:
    RenderNodePostProcessUtil rn_;

    uint32_t refCount_ { 0U };
};
RENDER_END_NAMESPACE()

#endif // RENDER__RENDER_NODE_POST_PROCESS_UTIL_H
