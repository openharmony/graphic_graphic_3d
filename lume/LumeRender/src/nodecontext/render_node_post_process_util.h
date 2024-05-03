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

#ifndef RENDER_RENDER__RENDER_NODE_POST_PROCESS_UTIL_H
#define RENDER_RENDER__RENDER_NODE_POST_PROCESS_UTIL_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node_post_process_util.h>
#include <render/render_data_structures.h>

#include "node/render_bloom.h"
#include "node/render_blur.h"
#include "node/render_copy.h"
#include "node/render_motion_blur.h"

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

    void UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration);
    void ProcessPostProcessConfiguration();
    void InitCreateShaderResources();
    void InitCreateBinders();

    struct InputOutput {
        BindableImage input;
        BindableImage output;
    };
    void ExecuteCombine(IRenderCommandList& cmdList, const InputOutput& inOut);
    void ExecuteFXAA(IRenderCommandList& cmdList, const InputOutput& inOut);
    void ExecuteBlit(IRenderCommandList& cmdList, const InputOutput& inOut);
    void ExecuteTAA(IRenderCommandList& cmdList, const InputOutput& inOut);
    void ExecuteDofBlur(IRenderCommandList& cmdList, const InputOutput& inOut);
    void ExecuteDof(IRenderCommandList& cmdList, const InputOutput& inOut);

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
        RenderHandle shader;
        RenderHandle pl;
        RenderHandle pso;

        PipelineLayout pipelineLayout;
    };
    EffectData fxaaData_;
    EffectData taaData_;
    EffectData dofBlurData_;
    EffectData dofData_;
    EffectData copyData_;
    EffectData combineData_;

    RenderBloom renderBloom_;
    RenderBlur renderBlur_;
    RenderMotionBlur renderMotionBlur_;
    RenderCopy renderCopy_;
    // additional layer copy
    RenderCopy renderCopyLayer_;

    RenderBlur renderNearBlur_;
    RenderBlur renderFarBlur_;

    struct AdditionalImageData {
        RenderHandle black;
        RenderHandle white;
    };
    IRenderNodePostProcessUtil::ImageData images_;
    AdditionalImageData aimg_;

    IRenderNodePostProcessUtil::PostProcessInfo postProcessInfo_;

    enum POST_PROCESS_UBO_INDICES {
        PP_TAA_IDX = 0U,
        PP_BLOOM_IDX,
        PP_BLUR_IDX,
        PP_FXAA_IDX,
        PP_COMBINED_IDX,
        PP_DOF_IDX,
        PP_MB_IDX,
        PP_COUNT_IDX,
    };

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RenderHandleReference postProcess;
    };
    UboHandles ubos_;

    BASE_NS::Math::UVec2 outputSize_ { 0U, 0U };

    bool validInputs_ { true };
    bool validInputsForTaa_ { false };
    bool validInputsForDof_ { false };
    bool validInputsForMb_ { false };
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
        IDescriptorSetBinder::Ptr globalSet0[POST_PROCESS_UBO_INDICES::PP_COUNT_IDX];

        IDescriptorSetBinder::Ptr combineBinder;
        IDescriptorSetBinder::Ptr taaBinder;
        IDescriptorSetBinder::Ptr fxaaBinder;
        IDescriptorSetBinder::Ptr dofBlurBinder;
        IDescriptorSetBinder::Ptr dofBinder;

        IDescriptorSetBinder::Ptr copyBinder;
    };
    AllBinders binders_;

    DeviceBackendType deviceBackendType_ { DeviceBackendType::VULKAN };
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
