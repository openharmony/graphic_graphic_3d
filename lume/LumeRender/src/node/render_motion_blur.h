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

#ifndef RENDER_RENDER__NODE__RENDER_MOTION_BLUR_H
#define RENDER_RENDER__NODE__RENDER_MOTION_BLUR_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;

class RenderMotionBlur final {
public:
    RenderMotionBlur() = default;
    ~RenderMotionBlur() = default;

    struct MotionBlurInfo {
        RenderHandle input;
        RenderHandle output;
        RenderHandle velocity;
        RenderHandle depth;
        RenderHandle globalUbo;
        uint32_t globalUboByteOffset { 0U };
        BASE_NS::Math::UVec2 size { 0U, 0U };
    };
    void Init(IRenderNodeContextManager& renderNodeContextMgr, const MotionBlurInfo& blurInfo);
    void PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const MotionBlurInfo& blurInfo,
        const PostProcessConfiguration& ppConfig);
    void Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig);

    DescriptorCounts GetDescriptorCounts() const;

private:
    void ExecuteTileVelocity(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig);
    void UpdateDescriptorSet0(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig);
    RenderHandle GetTileVelocityForMotionBlur() const;

    struct RenderDataHandles {
        RenderHandle shader;
        RenderHandle pso;
        PipelineLayout pipelineLayout;
    };
    RenderDataHandles renderData_;
    RenderDataHandles renderTileMaxData_;
    struct RenderDataHandlesTileNeighborhood {
        RenderHandle shader;
        PipelineLayout pipelineLayout;
        RenderHandle psoNeighborhood;
        RenderHandle psoHorizontal;
        RenderHandle psoVertical;
        bool doublePass { true };
    };
    RenderDataHandlesTileNeighborhood renderTileNeighborData_;

    IDescriptorSetBinder::Ptr globalSet0_;
    IDescriptorSetBinder::Ptr localSet1_;
    IDescriptorSetBinder::Ptr localTileMaxSet1_;
    IDescriptorSetBinder::Ptr localTileNeighborhoodSet1_[2U];
    RenderHandle samplerHandle_;
    RenderHandle samplerNearestHandle_;
    MotionBlurInfo motionBlurInfo_;

    RenderHandleReference tileVelocityImages_[2U];
    BASE_NS::Math::UVec2 tileImageSize_ { 0U, 0U };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_MOTION_BLUR_H
