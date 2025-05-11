/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CORE3D_RENDER_NODE_RENDER_NODE_GENERICS_H
#define CORE3D_RENDER_NODE_RENDER_NODE_GENERICS_H

#include <array>
#include <tuple>

#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/math/float_packer.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/render_data_structures.h>

struct RenderSize {
    int32_t w { 0 };
    int32_t h { 0 };
    constexpr explicit RenderSize(int32_t width, int32_t height) : w(width), h(height) {};
    constexpr RenderSize Downscale(int factor)
    {
        return RenderSize(w >> factor, h >> factor);
    }
};

template<typename T>
uint8_t* begin(T& s)
{
    return reinterpret_cast<uint8_t*>(&s);
}

template<typename T>
const uint8_t* cbegin(const T& s)
{
    return reinterpret_cast<const uint8_t*>(&s);
}

template<typename T>
const uint8_t* cend(const T& s)
{
    return reinterpret_cast<const uint8_t*>(&s) + sizeof(s);
}

struct Pass {
    const BASE_NS::Math::UVec2 size;
    RENDER_NS::AttachmentLoadOp op;
    const RENDER_NS::RenderHandle input;
    const RENDER_NS::RenderHandle output;
    const RENDER_NS::RenderHandle depth;
    const RENDER_NS::RenderHandle sampler;

    operator RENDER_NS::RenderPass() const
    {
        using namespace RENDER_NS;
        RenderPass rp;
        rp.renderPassDesc.attachmentCount = 1u;
        rp.renderPassDesc.attachmentHandles[0u] = output;
        rp.renderPassDesc.attachments[0u].loadOp = op;
        rp.renderPassDesc.attachments[0u].clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
        rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        rp.renderPassDesc.renderArea = { 0, 0, size.x, size.y };
        rp.renderPassDesc.subpassCount = 1u;
        rp.subpassDesc.colorAttachmentCount = 1u;
        rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
        rp.subpassStartIndex = 0u;
        return rp;
    }
};

struct ExplicitPass {
    BASE_NS::Math::UVec2 size;
    RENDER_NS::RenderPass p;
    operator RENDER_NS::RenderPass() const
    {
        return p;
    }
};

struct CommonShaderData {
    RENDER_NS::RenderHandle shader;
    RENDER_NS::RenderHandle pipelineLayout;
    RENDER_NS::RenderHandle pso;
    RENDER_NS::PipelineLayout pipelineLayoutData;
};

struct GraphicsShaderData : public CommonShaderData {
    RENDER_NS::RenderHandle graphicsState;
};

struct ComputeShaderData : public CommonShaderData {
    RENDER_NS::ShaderThreadGroup threadGroupSize;
};

template<typename T>
struct PushContant {
    const T& pc;
    operator BASE_NS::array_view<const uint8_t>() const
    {
        return { reinterpret_cast<const uint8_t*>(&pc), sizeof(pc) };
    }
};

template<typename T, class U = Pass>
struct BlurParams {
    const CommonShaderData& shader;
    RENDER_NS::IPipelineDescriptorSetBinder& binder;
    PushContant<T> pc;
    U data;
};

struct Guassian {
    BASE_NS::Math::Vec4 texSizeInvTexSize { 0.0f, 0.0f, 0.0f, 0.0f };
    BASE_NS::Math::Vec4 dir { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<uint32_t, 16U> packed {};

    static Guassian StandardWeights() noexcept
    {
        using namespace BASE_NS;
        static constexpr int BLUR_SEPARATE_COUNT = 3;
        static constexpr float offset[BLUR_SEPARATE_COUNT] = { 0.0f, 1.3846153846f, 3.2307692308f };
        static constexpr float weight[BLUR_SEPARATE_COUNT] = { 0.2270270270f, 0.3162162162f, 0.0702702703f };
        std::array<uint32_t, 16> packed = {};
        for (uint32_t i = 0; i < BLUR_SEPARATE_COUNT; ++i) {
            packed[i] = Math::PackHalf2X16(Math::Vec2(offset[i], weight[i]));
        }

        return Guassian { { 3.0f + 0.5f, 0.0f, 0.0f, 0.0f }, {}, packed };
    }
};

struct RenderNodeMixin {
    void Load(const BASE_NS::string_view shader, BASE_NS::vector<RENDER_NS::DescriptorCounts>& counts,
        ComputeShaderData& debugShaderData_, size_t reserved);
    void Load(const BASE_NS::string_view file, BASE_NS::vector<RENDER_NS::DescriptorCounts>& counts,
        GraphicsShaderData& debugShaderData_, BASE_NS::array_view<const RENDER_NS::DynamicStateEnum> dynstates,
        size_t reserved);
    RENDER_NS::RenderPass CreateRenderPass(const RENDER_NS::RenderHandle input);

    void RecreateImages(RenderSize size, RENDER_NS::RenderHandleReference& handle);
    void RecreateImages(RenderSize size, RENDER_NS::RenderHandleReference& handle, RENDER_NS::GpuImageDesc desc);

protected:
    virtual operator RENDER_NS::IRenderNodeContextManager*() const = 0;
    virtual ~RenderNodeMixin() = default;
};

template<typename T>
void UpdateBufferData(
    const RENDER_NS::IRenderNodeGpuResourceManager& gpuResourceMgr, RENDER_NS::RenderHandle handle, const T& settings)
{
    auto view = BASE_NS::array_view(cbegin(settings), cend(settings));
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(handle)); data) {
        BASE_NS::CloneData(data, view.size(), view.data(), view.size());
        gpuResourceMgr.UnmapBuffer(handle);
    }
}

#endif // CORE3D_RENDER_NODE_RENDER_NODE_GENERICS_H