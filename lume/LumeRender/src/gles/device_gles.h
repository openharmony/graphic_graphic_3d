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

#ifndef GLES_DEVICE_GLES_H
#define GLES_DEVICE_GLES_H

#include <cstddef>
#include <cstdint>
#include <mutex>

#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/namespace.h>
#include <render/device/pipeline_state_desc.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/device.h"
#include "gles/swapchain_gles.h"

#if RENDER_HAS_GLES_BACKEND
#include "egl_state.h"
#endif
#if RENDER_HAS_GL_BACKEND
#include "wgl_state.h"
#endif

RENDER_BEGIN_NAMESPACE()
class ComputePipelineStateObject;
class GpuBuffer;
class GpuComputeProgram;
class GpuImage;
class GpuResourceManager;
class GpuSemaphore;
class GpuSampler;
class GpuShaderProgram;
class GraphicsPipelineStateObject;
class LowLevelDeviceGLES;
class NodeContextDescriptorSetManager;
class NodeContextPoolManager;
class RenderBackend;
class RenderFrameSync;
class PlatformGpuMemoryAllocator;
class ShaderManager;
class Swapchain;
class SwapchainGLES;

struct BackendSpecificImageDesc;
struct GpuAccelerationStructureDesc;
struct GpuBufferDesc;
struct GpuComputeProgramCreateData;
struct GpuImageDesc;
struct GpuImagePlatformData;
struct GpuSamplerDesc;
struct GpuShaderProgramCreateData;
struct SwapchainCreateInfo;
struct PipelineLayout;

class DeviceGLES final : public Device {
public:
    // NOTE: normalized flag
    struct ImageFormat {
        BASE_NS::Format coreFormat;
        uint32_t format;
        uint32_t internalFormat;
        uint32_t dataType;
        uint32_t bytesperpixel;
        struct {
            bool compressed;
            uint8_t blockW;
            uint8_t blockH;
            uint32_t bytesperblock;
        } compression;
        BASE_NS::Math::UVec4 swizzle;
    };

    DeviceGLES(RenderContext& renderContext, DeviceCreateInfo const& createInfo);
    ~DeviceGLES() override;

    // From IDevice
    DeviceBackendType GetBackendType() const override;
    const DevicePlatformData& GetPlatformData() const override;
    AccelerationStructureBuildSizes GetAccelerationStructureBuildSizes(
        const AccelerationStructureBuildGeometryInfo& geometry,
        BASE_NS::array_view<const AccelerationStructureGeometryTrianglesInfo> triangles,
        BASE_NS::array_view<const AccelerationStructureGeometryAabbsInfo> aabbs,
        BASE_NS::array_view<const AccelerationStructureGeometryInstancesInfo> instances) const override;
    FormatProperties GetFormatProperties(BASE_NS::Format format) const override;
    ILowLevelDevice& GetLowLevelDevice() const override;
    // NOTE: can be called from API
    void WaitForIdle() override;

    PlatformGpuMemoryAllocator* GetPlatformGpuMemoryAllocator() override;

    // (re-)create swapchain
    BASE_NS::unique_ptr<Swapchain> CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo) override;
    void DestroyDeviceSwapchain() override;

    bool IsActive() const;
    void Activate() override;
    void Deactivate() override;

    bool AllowThreadedProcessing() const override;

    GpuQueue GetValidGpuQueue(const GpuQueue& gpuQueue) const override;
    uint32_t GetGpuQueueCount() const override;

    void InitializePipelineCache(BASE_NS::array_view<const uint8_t> initialData) override;
    BASE_NS::vector<uint8_t> GetPipelineCache() const override;

    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuBufferDesc& desc) override;
    BASE_NS::unique_ptr<GpuBuffer> CreateGpuBuffer(const GpuAccelerationStructureDesc& desc) override;

    // Create gpu image resources
    BASE_NS::unique_ptr<GpuImage> CreateGpuImage(const GpuImageDesc& desc) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const GpuImagePlatformData& platformData) override;
    BASE_NS::unique_ptr<GpuImage> CreateGpuImageView(
        const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData) override;
    BASE_NS::vector<BASE_NS::unique_ptr<GpuImage>> CreateGpuImageViews(const Swapchain& platformSwapchain) override;

    BASE_NS::unique_ptr<GpuSampler> CreateGpuSampler(const GpuSamplerDesc& desc) override;

    BASE_NS::unique_ptr<RenderFrameSync> CreateRenderFrameSync() override;

    BASE_NS::unique_ptr<RenderBackend> CreateRenderBackend(
        GpuResourceManager& gpuResourceMgr, const CORE_NS::IParallelTaskQueue::Ptr& queue) override;

    BASE_NS::unique_ptr<ShaderModule> CreateShaderModule(const ShaderModuleCreateInfo& data) override;
    BASE_NS::unique_ptr<ShaderModule> CreateComputeShaderModule(const ShaderModuleCreateInfo& data) override;
    BASE_NS::unique_ptr<GpuShaderProgram> CreateGpuShaderProgram(const GpuShaderProgramCreateData& data) override;
    BASE_NS::unique_ptr<GpuComputeProgram> CreateGpuComputeProgram(const GpuComputeProgramCreateData& data) override;

    BASE_NS::unique_ptr<NodeContextDescriptorSetManager> CreateNodeContextDescriptorSetManager() override;
    BASE_NS::unique_ptr<NodeContextPoolManager> CreateNodeContextPoolManager(
        class GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue) override;

    BASE_NS::unique_ptr<GraphicsPipelineStateObject> CreateGraphicsPipelineStateObject(
        const GpuShaderProgram& gpuProgram, const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
        const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<ComputePipelineStateObject> CreateComputePipelineStateObject(
        const GpuComputeProgram& gpuProgram, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData* pipelineLayoutData) override;

    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphore() override;
    BASE_NS::unique_ptr<GpuSemaphore> CreateGpuSemaphoreView(const uint64_t handle) override;

    void SetBackendConfig(const BackendConfig& config) override;

    // Internal apis, only usable by other parts of GLES backend.
    bool HasExtension(BASE_NS::string_view extension) const;
    void Activate(RenderHandle swapchain);
#if RENDER_HAS_GL_BACKEND
    const WGLHelpers::WGLState& GetEglState();
#endif
#if RENDER_HAS_GLES_BACKEND
    const EGLHelpers::EGLState& GetEglState();
    bool IsDepthResolveSupported() const;
#endif

    uint32_t CacheProgram(
        BASE_NS::string_view vertSource, BASE_NS::string_view fragSource, BASE_NS::string_view compSource);
    void ReleaseProgram(uint32_t program);

    void UseProgram(uint32_t program);
    void BindBuffer(uint32_t target, uint32_t buffer);
    void BindBufferRange(uint32_t target, uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t size);
    void BindSampler(uint32_t textureUnit, uint32_t sampler);
    void BindTexture(uint32_t textureUnit, uint32_t target, uint32_t texture); // target = GL_TEXTURE_2D et al.
    void BindImageTexture(uint32_t unit, uint32_t texture, uint32_t level, bool layered, uint32_t layer,
        uint32_t access, uint32_t format);
    void BindFrameBuffer(uint32_t fbo);
    void BindReadFrameBuffer(uint32_t fbo);
    void BindWriteFrameBuffer(uint32_t fbo);
    void BindVertexArray(uint32_t vao);

    void BindVertexBuffer(uint32_t slot, uint32_t buffer, intptr_t offset, intptr_t stride);
    void VertexBindingDivisor(uint32_t slot, uint32_t divisor);
    void BindElementBuffer(uint32_t buffer);

    uint32_t BoundReadFrameBuffer() const;
    uint32_t BoundWriteFrameBuffer() const;

    uint32_t BoundProgram() const;
    uint32_t BoundBuffer(uint32_t target) const;
    uint32_t BoundBuffer(uint32_t target, uint32_t binding) const;
    uint32_t BoundSampler(uint32_t textureUnit) const;
    uint32_t BoundTexture(uint32_t textureUnit, uint32_t target) const;
    uint32_t BoundVertexArray() const;

    // Creation functions for objects.
    uint32_t CreateVertexArray();
    // Deletion functions for objects.
    void DeleteTexture(uint32_t texture);
    void DeleteBuffer(uint32_t buffer);
    void DeleteSampler(uint32_t sampler);
    void DeleteVertexArray(uint32_t vao);
    void DeleteFrameBuffer(uint32_t fbo);

    void SetActiveTextureUnit(uint32_t textureUnit); // hide this.
    // swizzles for textures
    void TexSwizzle(uint32_t image, uint32_t target, const BASE_NS::Math::UVec4& swizzle);
    // texture upload / storage
    void TexStorage2D(
        uint32_t image, uint32_t target, uint32_t levels, uint32_t internalformat, const BASE_NS::Math::UVec2& extent);
    void TexStorage3D(
        uint32_t image, uint32_t target, uint32_t levels, uint32_t internalformat, const BASE_NS::Math::UVec3& extent);
    void TexStorage2DMultisample(uint32_t image, uint32_t target, uint32_t samples, uint32_t internalformat,
        const BASE_NS::Math::UVec2& extent, bool fixedsamplelocations);

    void TexSubImage2D(uint32_t image, uint32_t target, uint32_t level, const BASE_NS::Math::UVec2& offset,
        const BASE_NS::Math::UVec2& extent, uint32_t format, uint32_t type, const void* pixels);
    void TexSubImage3D(uint32_t image, uint32_t target, uint32_t level, const BASE_NS::Math::UVec3& offset,
        const BASE_NS::Math::UVec3& extent, uint32_t format, uint32_t type, const void* pixels);
    void CompressedTexSubImage2D(uint32_t image, uint32_t target, uint32_t level, const BASE_NS::Math::UVec2& offset,
        const BASE_NS::Math::UVec2& extent, uint32_t format, uint32_t imageSize, const void* data);
    void CompressedTexSubImage3D(uint32_t image, uint32_t target, uint32_t level, const BASE_NS::Math::UVec3& offset,
        const BASE_NS::Math::UVec3& extent, uint32_t format, uint32_t imageSize, const void* data);

    const ImageFormat& GetGlImageFormat(BASE_NS::Format format) const;

    void SwapBuffers(const SwapchainGLES&);

private:
    enum BufferBindId : uint32_t {
        UNIFORM_BUFFER_BIND = 0,
        SHADER_STORAGE_BUFFER_BIND,
#ifdef HANDLE_UNSUPPORTED_ENUMS
        ATOMIC_COUNTER_BUFFER_BIND,
        TRANSFORM_FEEDBACK_BUFFER_BIND,
#endif
        MAX_BUFFER_BIND_ID
    };

    enum BufferTargetId : uint32_t {
        PIXEL_UNPACK_BUFFER = 0,
        PIXEL_PACK_BUFFER,
        COPY_READ_BUFFER,
        COPY_WRITE_BUFFER,
        UNIFORM_BUFFER,
        SHADER_STORAGE_BUFFER,
        DISPATCH_INDIRECT_BUFFER,
        DRAW_INDIRECT_BUFFER,
#ifdef HANDLE_UNSUPPORTED_ENUMS
        ATOMIC_COUNTER_BUFFER,
        QUERY_BUFFER,
        TRANSFORM_FEEDBACK_BUFFER,
        ARRAY_BUFFER,
        ELEMENT_ARRAY_BUFFER, // stored in VAO state...
        TEXTURE_BUFFER,
#endif
        MAX_BUFFER_TARGET_ID
    };

    enum TextureTargetId : uint32_t {
        TEXTURE_2D = 0,
        TEXTURE_CUBE_MAP = 1,
#if RENDER_HAS_GLES_BACKEND
        TEXTURE_EXTERNAL_OES = 2,
#endif
        TEXTURE_2D_MULTISAMPLE = 3,
        TEXTURE_2D_ARRAY = 4,
        TEXTURE_3D = 5,
        MAX_TEXTURE_TARGET_ID
    };

    static constexpr uint32_t READ_ONLY { 0x88B8 }; /* GL_READ_ONLY */
    static constexpr uint32_t R32UI { 0x8236 };     /* GL_R32UI */
    static constexpr uint32_t MAX_TEXTURE_UNITS { 16 };
    static constexpr uint32_t MAX_SAMPLERS { 16 };
    static constexpr uint32_t MAX_BOUND_IMAGE { 16 };
    static constexpr uint32_t MAX_BINDING_VALUE { 16 };

    // Cleanup cache state when deleting objects.
    void UnBindTexture(uint32_t texture);
    void UnBindBuffer(uint32_t buffer);
    void UnBindBufferFromVertexArray(uint32_t buffer);
    void UnBindSampler(uint32_t sampler);
    void UnBindVertexArray(uint32_t vao);
    void UnBindFrameBuffer(uint32_t fbo);

    BASE_NS::vector<BASE_NS::string_view> extensions_;
    BASE_NS::vector<ImageFormat> supportedFormats_;

    enum { VERTEX_CACHE = 0, FRAGMENT_CACHE = 1, COMPUTE_CACHE = 2, MAX_CACHES };
    struct ShaderCache {
        size_t hit { 0 };
        size_t miss { 0 };
        struct Entry {
            uint32_t shader { 0 };
            uint64_t hash { 0 }; // hash of generated GLSL
            uint32_t refCount { 0 };
        };
        BASE_NS::vector<Entry> cache;
    };
    ShaderCache caches[MAX_CACHES];

    const ShaderCache::Entry& CacheShader(int type, BASE_NS::string_view source);
    void ReleaseShader(uint32_t type, uint32_t shader);

    struct ProgramCache {
        uint32_t program { 0 };
        uint32_t vertShader { 0 };
        uint32_t fragShader { 0 };
        uint32_t compShader { 0 };
        uint64_t hashVert { 0 };
        uint64_t hashFrag { 0 };
        uint64_t hashComp { 0 };
        uint32_t refCount { 0 };
    };
    BASE_NS::vector<ProgramCache> programs_;
    size_t pCacheHit_ { 0 };
    size_t pCacheMiss_ { 0 };

#if RENDER_HAS_GL_BACKEND
#if _WIN32
    const DeviceBackendType backendType_ = DeviceBackendType::OPENGL;
    WGLHelpers::WGLState eglState_;
#else
#error Core::DeviceBackendType::OPENGL not implemented for this platform yet.
#endif
#endif
#if RENDER_HAS_GLES_BACKEND
    const DeviceBackendType backendType_ = DeviceBackendType::OPENGLES;
    EGLHelpers::EGLState eglState_;
    BackendConfigGLES backendConfig_ { {}, false };
#endif
    mutable std::mutex activeMutex_;
    uint32_t isActive_ { 0 };
    bool isRenderbackendRunning_ { false };
    BASE_NS::unique_ptr<LowLevelDeviceGLES> lowLevelDevice_;
    // GL State cache..
    // cache.
    uint32_t activeTextureUnit_ = { 0 };
    uint32_t boundSampler_[MAX_SAMPLERS] = { 0 };

    struct {
        bool bound { false };
        uint32_t texture { 0 };
        uint32_t level { 0 };
        bool layered { false };
        uint32_t layer { 0 };
        uint32_t access { READ_ONLY };
        uint32_t format { R32UI };
    } boundImage_[MAX_BOUND_IMAGE] = { {} };

    uint32_t boundTexture_[MAX_TEXTURE_UNITS][MAX_TEXTURE_TARGET_ID] = { { 0 } }; // [textureunit][target type]

    struct BufferCache {
        bool cached { false };
        uint32_t buffer { 0 };
        uint64_t offset { 0 };
        uint64_t size { 0 };
    };
    // Cache for GL_ATOMIC_COUNTER_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, GL_UNIFORM_BUFFER, or GL_SHADER_STORAGE_BUFFER
    // bindings.
    BufferCache boundBuffers_[MAX_BUFFER_BIND_ID][MAX_BINDING_VALUE] = { {} };

    // bufferBound_ caches GL_PIXEL_UNPACK_BUFFER / GL_COPY_READ_BUFFER / GL_COPY_WRITE_BUFFER (and other generic
    // bindings)
    struct {
        bool bound { false };
        uint32_t buffer { 0 };
    } bufferBound_[MAX_BUFFER_TARGET_ID];
    uint32_t boundReadFbo_ { 0 };
    uint32_t boundWriteFbo_ { 0 };
    uint32_t boundVao_ { 0 };
    uint32_t boundProgram_ { 0 };
    struct VAOState {
        uint32_t vao { 0 }; // GL name for object.
        struct {
            bool bound { false };
            uint32_t buffer { 0 };
        } elementBuffer;
        struct {
            bool bound { false };
            uint32_t buffer { 0 };
            intptr_t offset { 0 };
            intptr_t stride { 0 };
            uint32_t divisor { 0 };
        } vertexBufferBinds[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
    };
    BASE_NS::vector<VAOState> vaoStates_;
    size_t vaoStatesInUse_ = 0;

    // glid -> internal id
    static BufferBindId IndexedTargetToTargetId(uint32_t target);
    static uint32_t TargetToBinding(uint32_t target);
    static BufferTargetId GenericTargetToTargetId(uint32_t target);
    static TextureTargetId TextureTargetToTargetId(uint32_t target);

    // internal id -> glid
    static uint32_t GenericTargetIdToTarget(BufferTargetId target);
    static uint32_t IndexedTargetIdToTarget(BufferBindId target);
    static uint32_t TextureTargetIdToTarget(TextureTargetId target);
};

// Wrapper for low level device access
class LowLevelDeviceGLES final : public ILowLevelDeviceGLES {
public:
    explicit LowLevelDeviceGLES(DeviceGLES& deviceGLES);
    ~LowLevelDeviceGLES() = default;

    DeviceBackendType GetBackendType() const override;

#if RENDER_HAS_EXPERIMENTAL
    void Activate() override;
    void Deactivate() override;
    void SwapBuffers() override;
#endif

private:
    DeviceGLES& deviceGLES_;
    GpuResourceManager& gpuResourceMgr_;
};

#if (RENDER_HAS_GL_BACKEND)
BASE_NS::unique_ptr<Device> CreateDeviceGL(RenderContext& renderContext, DeviceCreateInfo const& createInfo);
#endif
#if (RENDER_HAS_GLES_BACKEND)
BASE_NS::unique_ptr<Device> CreateDeviceGLES(RenderContext& renderContext, DeviceCreateInfo const& createInfo);
#endif
RENDER_END_NAMESPACE()

#endif // GLES_DEVICE_GLES_H
