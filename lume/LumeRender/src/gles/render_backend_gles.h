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

#ifndef GLES_RENDER_BACKEND_GLES_H
#define GLES_RENDER_BACKEND_GLES_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <render/namespace.h>

#include "nodecontext/render_command_list.h"
#include "render_backend.h"
#if defined(RENDER_PERF_ENABLED) && (RENDER_PERF_ENABLED == 1)
#include <atomic>

#include "device/gpu_buffer.h"
#include "perf/cpu_timer.h"
#include "perf/gpu_query_manager.h"
#endif

RENDER_BEGIN_NAMESPACE()
class GpuShaderProgramGLES;
class GpuComputeProgramGLES;
class GpuSamplerGLES;
class GpuImageGLES;
class GpuBufferGLES;
class Device;
class DeviceGLES;
class GpuResourceManager;
class NodeContextPsoManager;
class NodeContextPoolManager;
class RenderBarrierList;
class RenderCommandList;
struct RenderCommandContext;
struct LowLevelCommandBuffer;
struct LowlevelFramebufferGL;
class ComputePipelineStateObjectGLES;
class GraphicsPipelineStateObjectGLES;
struct NodeGraphBackBufferConfiguration;
struct OES_Bind;
namespace Gles {
struct CombinedSamplerInfo;
struct PushConstantReflection;
struct BindMaps;
struct Bind;
} // namespace Gles
struct GpuBufferPlatformDataGL;
struct GpuSamplerPlatformDataGL;
struct GpuImagePlatformDataGL;
/**
RenderBackGLES.
OpenGLES 3.2+ render backend.
(also used for OpenGL 4.5+ on desktop)
**/
class RenderBackendGLES final : public RenderBackend {
public:
    RenderBackendGLES(Device& device, GpuResourceManager& gpuResourceManager);
    ~RenderBackendGLES();

    void Render(RenderCommandFrameData& renderCommandFrameData,
        const RenderBackendBackBufferConfiguration& backBufferConfig) override;
    void Present(const RenderBackendBackBufferConfiguration& backBufferConfig) override;

private:
    void RenderSingleCommandList(const RenderCommandContext& renderCommandCtx);
    void RenderProcessEndCommandLists(
        RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig);

    void RenderCommandDraw(const RenderCommandWithType&);
    void RenderCommandDrawIndirect(const RenderCommandWithType&);
    void RenderCommandDispatch(const RenderCommandWithType&);
    void RenderCommandDispatchIndirect(const RenderCommandWithType&);
    void RenderCommandBindPipeline(const RenderCommandWithType&);
    void BindGraphicsPipeline(const struct RenderCommandBindPipeline&);
    void BindComputePipeline(const struct RenderCommandBindPipeline&);
    void RenderCommandBindVertexBuffers(const RenderCommandWithType&);
    void RenderCommandBindIndexBuffer(const RenderCommandWithType&);
    void RenderCommandBeginRenderPass(const RenderCommandWithType&);
    void RenderCommandNextSubpass(const RenderCommandWithType&);
    void RenderCommandEndRenderPass(const RenderCommandWithType&);
    void RenderCommandCopyBuffer(const RenderCommandWithType&);
    void RenderCommandCopyBufferImage(const RenderCommandWithType&);
    void RenderCommandCopyImage(const RenderCommandWithType&);
    void RenderCommandBlitImage(const RenderCommandWithType&);
    void RenderCommandBarrierPoint(const RenderCommandWithType&);
    void RenderCommandBindDescriptorSets(const RenderCommandWithType&);
    void RenderCommandPushConstant(const RenderCommandWithType&);
    void RenderCommandClearColorImage(const RenderCommandWithType&);
    void RenderCommandDynamicStateViewport(const RenderCommandWithType&);
    void RenderCommandDynamicStateScissor(const RenderCommandWithType&);
    void RenderCommandDynamicStateLineWidth(const RenderCommandWithType&);
    void RenderCommandDynamicStateDepthBias(const RenderCommandWithType&);
    void RenderCommandDynamicStateBlendConstants(const RenderCommandWithType&);
    void RenderCommandDynamicStateDepthBounds(const RenderCommandWithType&);
    void RenderCommandDynamicStateStencil(const RenderCommandWithType&);
    void RenderCommandFragmentShadingRate(const RenderCommandWithType& renderCmd);
    void RenderCommandExecuteBackendFramePosition(const RenderCommandWithType&);
    void RenderCommandWriteTimestamp(const RenderCommandWithType&);
    void RenderCommandUndefined(const RenderCommandWithType&);
    typedef void (RenderBackendGLES::*RenderCommandHandler)(const RenderCommandWithType&);
    // Following array must match in order of RenderCommandType
    // Count of render command types
    static constexpr uint32_t RENDER_COMMAND_COUNT = (static_cast<uint32_t>(RenderCommandType::COUNT));
    static constexpr RenderCommandHandler COMMAND_HANDLERS[RENDER_COMMAND_COUNT] = {
        &RenderBackendGLES::RenderCommandUndefined, &RenderBackendGLES::RenderCommandDraw,
        &RenderBackendGLES::RenderCommandDrawIndirect, &RenderBackendGLES::RenderCommandDispatch,
        &RenderBackendGLES::RenderCommandDispatchIndirect, &RenderBackendGLES::RenderCommandBindPipeline,
        &RenderBackendGLES::RenderCommandBeginRenderPass, &RenderBackendGLES::RenderCommandNextSubpass,
        &RenderBackendGLES::RenderCommandEndRenderPass, &RenderBackendGLES::RenderCommandBindVertexBuffers,
        &RenderBackendGLES::RenderCommandBindIndexBuffer, &RenderBackendGLES::RenderCommandCopyBuffer,
        &RenderBackendGLES::RenderCommandCopyBufferImage, &RenderBackendGLES::RenderCommandCopyImage,
        &RenderBackendGLES::RenderCommandBlitImage, &RenderBackendGLES::RenderCommandBarrierPoint,
        &RenderBackendGLES::RenderCommandBindDescriptorSets, &RenderBackendGLES::RenderCommandPushConstant,
        &RenderBackendGLES::RenderCommandUndefined, /* RenderCommandBuildAccelerationStructure */
        &RenderBackendGLES::RenderCommandClearColorImage, &RenderBackendGLES::RenderCommandDynamicStateViewport,
        &RenderBackendGLES::RenderCommandDynamicStateScissor, &RenderBackendGLES::RenderCommandDynamicStateLineWidth,
        &RenderBackendGLES::RenderCommandDynamicStateDepthBias,
        &RenderBackendGLES::RenderCommandDynamicStateBlendConstants,
        &RenderBackendGLES::RenderCommandDynamicStateDepthBounds, &RenderBackendGLES::RenderCommandDynamicStateStencil,
        &RenderBackendGLES::RenderCommandFragmentShadingRate,
        &RenderBackendGLES::RenderCommandExecuteBackendFramePosition, &RenderBackendGLES::RenderCommandWriteTimestamp,
        &RenderBackendGLES::RenderCommandUndefined, /* RenderCommandGpuQueueTransferRelease */
        &RenderBackendGLES::RenderCommandUndefined  /* RenderCommandGpuQueueTransferAcquire */
    };
    void PrimeCache(const GraphicsState& graphicsState); // Forces the graphics state..
    void PrimeDepthStencilState(const GraphicsState& graphicsState);
    void PrimeBlendState(const GraphicsState& graphicsState);
    void DoGraphicsState(const GraphicsState& graphicsState);
    void SetViewport(const RenderPassDesc::RenderArea& ra, const ViewportDesc& vd);
    void SetScissor(const RenderPassDesc::RenderArea& ra, const ScissorDesc& sd);

    void HandleColorAttachments(const BASE_NS::array_view<const RenderPassDesc::AttachmentDesc*> colorAttachments);
    void HandleDepthAttachment(const RenderPassDesc::AttachmentDesc& depthAttachment);

    void ClearScissorInit(const RenderPassDesc::RenderArea& aArea);
    void ClearScissorSet();
    void ClearScissorReset();
    static void SetPushConstant(uint32_t program, const Gles::PushConstantReflection& pc, const void* data);
    void SetPushConstants(uint32_t program, const BASE_NS::array_view<Gles::PushConstantReflection>& pushConstants);
    void DoSubPass(uint32_t subPass);

    struct Managers {
        NodeContextPsoManager* psoMgr { nullptr };
        NodeContextPoolManager* poolMgr { nullptr };
        NodeContextDescriptorSetManager* descriptorSetMgr { nullptr };
        const RenderBarrierList* rbList { nullptr };
    };
    Managers managers_;

    DeviceGLES& device_;
    GpuResourceManager& gpuResourceMgr_;

    struct PresentationInfo {
        uint32_t swapchainImageIndex { ~0u };
    };
    PresentationInfo presentationInfo_;

#if defined(RENDER_PERF_ENABLED) && (RENDER_PERF_ENABLED == 1)
    struct PerfCounters {
        uint32_t drawCount;
        uint32_t drawIndirectCount;
        uint32_t dispatchCount;
        uint32_t dispatchIndirectCount;

        uint32_t renderPassCount;

        uint32_t bindProgram;
        uint32_t bindSampler;
        uint32_t bindTexture;
        uint32_t bindBuffer;

        uint32_t triangleCount;
        uint32_t instanceCount;
    };
    PerfCounters perfCounters_;

    bool validGpuQueries_;
    BASE_NS::unique_ptr<GpuQueryManager> gpuQueryMgr_;
    struct PerfDataSet {
        EngineResourceHandle gpuHandle;
        CpuTimer cpuTimer;
        uint32_t counter;
    };
    BASE_NS::unordered_map<BASE_NS::string, PerfDataSet> timers_;

    void StartFrameTimers(const RenderCommandFrameData& renderCommandFrameData);
    void EndFrameTimers();
    void CopyPerfTimeStamp(BASE_NS::string_view aName, PerfDataSet& perfDataSet);

    struct CommonBackendCpuTimers {
        CpuTimer full;
        CpuTimer acquire;
        CpuTimer execute;
        CpuTimer submit;
        CpuTimer present;
    };
    CommonBackendCpuTimers commonCpuTimers_;

    std::atomic_int64_t fullGpuCounter_ { 0 };
#endif
    PolygonMode polygonMode_ = PolygonMode::CORE_POLYGON_MODE_FILL;
    PrimitiveTopology topology_ = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    struct ProgramState {
        bool setPushConstants { false };
        struct RenderCommandPushConstant pushConstants {};
    } boundProgram_;

    struct {
        uint32_t id { 0 };
        uintptr_t offset { 0 };
        IndexType type { CORE_INDEX_TYPE_UINT32 };
    } boundIndexBuffer_;

    void ResetState();
    void ResetBindings();
    void SetupCache(const PipelineLayout& pipelineLayout);
    struct BindState {
        bool dirty { false };
#if defined(RENDER_HAS_GLES_BACKEND) && (RENDER_HAS_GLES_BACKEND == 1)
        BASE_NS::vector<OES_Bind> oesBinds;
#endif
        BASE_NS::vector<Gles::Bind> resources;
    };
    BindState boundObjects_[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];

    static Gles::Bind& SetupBind(const DescriptorSetLayoutBinding& res, BASE_NS::vector<Gles::Bind>& resources);
    void BindSampler(const BindableSampler& res, Gles::Bind& obj, uint32_t index);
    void BindImage(const BindableImage& res, const GpuResourceState& resState, Gles::Bind& obj, uint32_t index);
    void BindImageSampler(const BindableImage& res, const GpuResourceState& resState, Gles::Bind& obj, uint32_t index);
    void BindBuffer(const BindableBuffer& res, Gles::Bind& obj, uint32_t dynamicOffset, uint32_t index);
    void BindVertexInputs(
        const VertexInputDeclarationData& decldata, const BASE_NS::array_view<const int32_t>& vertexInputs);
    void ProcessBindings(const struct RenderCommandBindDescriptorSets& renderCmd,
        const DescriptorSetLayoutBindingResources& data, uint32_t set);
    void ScanPasses(const RenderPassDesc& rpd);
    int32_t InvalidateColor(BASE_NS::array_view<uint32_t> invalidateAttachment, const RenderPassDesc& rpd,
        const RenderPassSubpassDesc& currentSubPass);
    int32_t InvalidateDepthStencil(BASE_NS::array_view<uint32_t> invalidateAttachment, const RenderPassDesc& rpd,
        const RenderPassSubpassDesc& currentSubPass);
    uint32_t ResolveMSAA(const RenderPassDesc& rpd, const RenderPassSubpassDesc& currentSubPass);
    void UpdateBlendState(const GraphicsState& graphicsState);
    void UpdateDepthState(const GraphicsState& graphicsState);
    void UpdateStencilState(const GraphicsState& graphicsState);
    void UpdateDepthStencilState(const GraphicsState& graphicsState);
    void UpdateRasterizationState(const GraphicsState& graphicsState);
    void BindResources();
    enum StencilSetFlags { SETOP = 1, SETCOMPAREMASK = 2, SETCOMPAREOP = 4, SETREFERENCE = 8, SETWRITEMASK = 16 };
    void SetStencilState(const uint32_t frontFlags, const GraphicsState::StencilOpState& front,
        const uint32_t backFlags, const GraphicsState::StencilOpState& back);
    const ComputePipelineStateObjectGLES* boundComputePipeline_ = nullptr;
    const GraphicsPipelineStateObjectGLES* boundGraphicsPipeline_ = nullptr;
    RenderHandle currentPsoHandle_;
    RenderPassDesc::RenderArea renderArea_;
    struct RenderCommandBeginRenderPass activeRenderPass_;
    uint32_t currentSubPass_ = 0;
    const LowlevelFramebufferGL* currentFrameBuffer_ = nullptr;
    // caching state
    GraphicsState cacheState_;
    bool cachePrimed_ = false;
    DynamicStateFlags dynamicStateFlags_ = DynamicStateFlagBits::CORE_DYNAMIC_STATE_UNDEFINED;
    bool viewportPrimed_ = false;
    bool clearScissorSet_ = false;
    bool resetScissor_ = false;
    RenderPassDesc::RenderArea clearScissor_;
    bool scissorPrimed_ = false;
    ScissorDesc scissorBox_;
    ViewportDesc viewport_;

    static constexpr uint32_t MAX_VERTEXINPUT_BINDINGS { 16 };
    uint32_t vertexAttribBinds_ = 0;
    struct {
        uint32_t id = 0;
        intptr_t offset = 0;
    } vertexAttribBindSlots_[MAX_VERTEXINPUT_BINDINGS];

    // attachments are cleared on first use, and marked as cleared here.
    bool attachmentCleared_[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] = { false };
    // subpass index where attachment is first used. (just for book keeping for now. clearing need is handled by
    // "attachmentCleared" )
    uint32_t attachmentFirstUse_[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] = { 0xFFFFFFFF };
    // subpass index where attachment is last used. (for invalidation purposes)
    uint32_t attachmentLastUse_[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] = { 0 };

    // will the attachment be resolved to backbuffer.. (if so flip coordinates)
    bool resolveToBackbuffer_[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] = { false };
    const GpuImageGLES* attachmentImage_[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] { nullptr };
    bool multisampledRenderToTexture_ = false;

    uint32_t blitImageSourceFbo_ { 0 };
    uint32_t blitImageDestinationFbo_ { 0 };
    uint32_t inRenderpass_ { 0 };
    bool scissorViewportSetDefaultFbo_ { false };
    bool renderingToDefaultFbo_ { false };
    bool scissorEnabled_ { false };
    bool scissorBoxUpdated_ { false };
    bool viewportDepthRangeUpdated_ { false };
    bool viewportUpdated_ { false };
    bool commandListValid_ { false };
    void FlushViewportScissors();

    void BufferToImageCopy(const struct RenderCommandCopyBufferImage& renderCmd);
    void ImageToBufferCopy(const struct RenderCommandCopyBufferImage& renderCmd);
#if defined(RENDER_HAS_GLES_BACKEND) && (RENDER_HAS_GLES_BACKEND == 1)
    BASE_NS::vector<OES_Bind> oesBinds_;
#endif
};
RENDER_END_NAMESPACE()

#endif // GLES_RENDER_BACKEND_GLES_H
