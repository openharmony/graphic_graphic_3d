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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOWS_BLUR_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOWS_BLUR_H

#include <3d/render/intf_render_data_store_default_light.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class RenderNodeDefaultShadowsBlur final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultShadowsBlur() = default;
    ~RenderNodeDefaultShadowsBlur() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "6bf65ac3-6ede-4baa-a61a-2207085b235c" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultShadowsBlur";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    struct TemporaryImage {
        RENDER_NS::RenderHandleReference imageHandle;
        uint32_t width { 0u };
        uint32_t height { 0u };
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        RENDER_NS::SampleCountFlags sampleCountFlags { RENDER_NS::SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT };
    };

    void ProcessSingleShadow(RENDER_NS::IRenderCommandList& cmdList, const uint32_t drawIdx,
        const RENDER_NS::RenderHandle imageHandle, const TemporaryImage& tempImage);
    void RenderData(RENDER_NS::IRenderCommandList& cmdList, const RENDER_NS::RenderPass& renderPassBase,
        const RENDER_NS::ViewportDesc& viewport, const RENDER_NS::ScissorDesc& scissor,
        const RENDER_NS::RenderHandle inputHandle, const RENDER_NS::RenderHandle outputHandle, const uint32_t drawIdx,
        const BASE_NS::Math::Vec4 texSizeInvTexSize);
    void RenderBlur(RENDER_NS::IRenderCommandList& cmdList, const RENDER_NS::RenderPass& renderPass,
        const RENDER_NS::ViewportDesc& viewport, const RENDER_NS::ScissorDesc& scissor,
        const RENDER_NS::IDescriptorSetBinder::Ptr& binder, const BASE_NS::Math::Vec4& texSizeInvTexSize,
        const BASE_NS::Math::Vec4& dir, RENDER_NS::RenderHandle imageHandle);
    void CreateDescriptorSets();

    void ExplicitInputBarrier(RENDER_NS::IRenderCommandList& cmdList, const RENDER_NS::RenderHandle handle);
    void ExplicitOutputBarrier(RENDER_NS::IRenderCommandList& cmdList, const RENDER_NS::RenderHandle handle);

    TemporaryImage temporaryImage_;
    RENDER_NS::RenderHandle shadowColorBufferHandle_;

    RENDER_NS::RenderHandle samplerHandle_;
    RENDER_NS::RenderHandle bufferHandle_;

    struct AllDescriptorSets {
        RENDER_NS::IDescriptorSetBinder::Ptr globalSet;

        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> set1Horizontal;
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> set1Vertical;
    };
    AllDescriptorSets allDescriptorSets_;

    struct ShaderData {
        RENDER_NS::RenderHandle shaderHandle;
        RENDER_NS::PushConstant pushConstant;

        RENDER_NS::RenderHandle psoHandle;
    };
    ShaderData shaderData_;

    SceneRenderDataStores stores_;

    IRenderDataStoreDefaultLight::ShadowTypes shadowTypes_;
    uint32_t shadowCount_ { 0U };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOWS_BLUR_H
