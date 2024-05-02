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

#ifndef DEVICE_SHADER_MANAGER_H
#define DEVICE_SHADER_MANAGER_H

#include <atomic>
#include <mutex>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_program.h"
#include "device/gpu_resource_handle_util.h" // for hash<RenderHandle>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class Device;
class ShaderModule;
class ShaderLoader;

constexpr const uint32_t INVALID_SM_INDEX { ~0u };

struct ComputeShaderCreateData {
    BASE_NS::string_view path;
    uint32_t renderSlotId { INVALID_SM_INDEX };
    uint32_t categoryId { INVALID_SM_INDEX };
    uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
    uint32_t shaderModuleIndex { INVALID_SM_INDEX };

    BASE_NS::string_view shaderFileStr;
    BASE_NS::string_view materialMetadata;
};

struct ShaderCreateData {
    BASE_NS::string_view path;
    uint32_t renderSlotId { INVALID_SM_INDEX };
    uint32_t categoryId { INVALID_SM_INDEX };
    uint32_t vertexInputDeclarationIndex { INVALID_SM_INDEX };
    uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
    uint32_t graphicsStateIndex { INVALID_SM_INDEX };

    uint32_t vertShaderModuleIndex { INVALID_SM_INDEX };
    uint32_t fragShaderModuleIndex { INVALID_SM_INDEX };

    BASE_NS::string_view shaderFileStr;
    BASE_NS::string_view materialMetadata;
};

struct ShaderReflectionData {
    BASE_NS::array_view<const uint8_t> reflectionData;

    bool IsValid() const;
    ShaderStageFlags GetStageFlags() const;
    PipelineLayout GetPipelineLayout() const;
    BASE_NS::vector<ShaderSpecialization::Constant> GetSpecializationConstants() const;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> GetInputDescriptions() const;
    BASE_NS::Math::UVec3 GetLocalSize() const;

    const uint8_t* GetPushConstants() const;
};

struct ShaderModuleCreateInfo {
    ShaderStageFlags shaderStageFlags;
    BASE_NS::array_view<const uint8_t> spvData;
    ShaderReflectionData reflectionData;
};

// Ref counted shaders are really not needed
// For API consistency the same RenderHandleReference is used, but the counter is dummy
class ShaderReferenceCounter final : public IRenderReferenceCounter {
public:
    ShaderReferenceCounter() = default;
    ~ShaderReferenceCounter() override = default;

    ShaderReferenceCounter(const ShaderReferenceCounter&) = delete;
    ShaderReferenceCounter& operator=(const ShaderReferenceCounter&) = delete;

    void Ref() override
    {
        refcnt_.fetch_add(1, std::memory_order_relaxed);
    };

    void Unref() override
    {
        if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    };

    int32_t GetRefCount() const override
    {
        // NOTE: not destroyed based on ref count
        // the manager is always holding the reference
        // basically plugins could hold their shader data handles and release
        return 1;
    };

private:
    std::atomic<int32_t> refcnt_ { 0 };
};

/* ShaderManager implementation.
Not internally synchronized. */
class ShaderManager final : public IShaderManager {
public:
    static constexpr uint32_t MAX_DEFAULT_NAME_LENGTH { 128 };

    explicit ShaderManager(Device& device);
    ~ShaderManager() override;

    RenderHandleReference Get(const RenderHandle& handle) const override;
    RenderHandleReference CreateComputeShader(const ComputeShaderCreateInfo& createInfo) override;
    RenderHandleReference CreateComputeShader(const ComputeShaderCreateInfo& createInfo,
        const BASE_NS::string_view baseShaderPath, const BASE_NS::string_view variantName) override;
    RenderHandleReference CreateShader(const ShaderCreateInfo& createInfo) override;
    RenderHandleReference CreateShader(const ShaderCreateInfo& createInfo, const BASE_NS::string_view baseShaderPath,
        const BASE_NS::string_view variantName) override;

    RenderHandleReference CreateVertexInputDeclaration(const VertexInputDeclarationCreateInfo& createInfo) override;
    RenderHandleReference CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) override;
    RenderHandleReference CreateGraphicsState(const GraphicsStateCreateInfo& createInfo) override;
    RenderHandleReference CreateGraphicsState(
        const GraphicsStateCreateInfo& createInfo, const GraphicsStateVariantCreateInfo& variantCreateInfo) override;

    uint32_t CreateRenderSlotId(const BASE_NS::string_view renderSlot) override;
    void SetRenderSlotData(const uint32_t renderSlotId, const RenderHandleReference& shaderHandle,
        const RenderHandleReference& stateHandle) override;

    uint32_t CreateCategoryId(const BASE_NS::string_view category);

    RenderHandleReference GetShaderHandle(const BASE_NS::string_view path) const override;
    RenderHandleReference GetShaderHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const override;
    RenderHandleReference GetShaderHandle(
        const RenderHandleReference& handle, const uint32_t renderSlotId) const override;
    RenderHandleReference GetShaderHandle(const RenderHandle& handle, const uint32_t renderSlotId) const;
    BASE_NS::vector<RenderHandleReference> GetShaders(const uint32_t renderSlotId) const override;
    BASE_NS::vector<RenderHandle> GetShaderRawHandles(const uint32_t renderSlotId) const;

    RenderHandleReference GetGraphicsStateHandle(const BASE_NS::string_view path) const override;
    RenderHandleReference GetGraphicsStateHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const override;
    RenderHandleReference GetGraphicsStateHandle(
        const RenderHandleReference& handle, const uint32_t renderSlotId) const override;
    RenderHandleReference GetGraphicsStateHandle(const RenderHandle& handle, const uint32_t renderSlotId) const;
    RenderHandleReference GetGraphicsStateHandleByHash(const uint64_t hash) const override;
    RenderHandleReference GetGraphicsStateHandleByShaderHandle(const RenderHandleReference& handle) const override;
    RenderHandleReference GetGraphicsStateHandleByShaderHandle(const RenderHandle& handle) const;
    GraphicsState GetGraphicsState(const RenderHandleReference& handle) const override;
    const GraphicsState& GetGraphicsStateRef(const RenderHandleReference& handle) const;
    const GraphicsState& GetGraphicsStateRef(const RenderHandle& handle) const;
    BASE_NS::vector<RenderHandleReference> GetGraphicsStates(const uint32_t renderSlotId) const override;

    uint32_t GetRenderSlotId(const BASE_NS::string_view renderSlot) const override;
    uint32_t GetRenderSlotId(const RenderHandleReference& handle) const override;
    uint32_t GetRenderSlotId(const RenderHandle& handle) const;
    RenderSlotData GetRenderSlotData(const uint32_t renderSlotId) const override;
    BASE_NS::string GetRenderSlotName(const uint32_t renderSlotId) const override;

    RenderHandleReference GetVertexInputDeclarationHandleByShaderHandle(
        const RenderHandleReference& handle) const override;
    RenderHandleReference GetVertexInputDeclarationHandleByShaderHandle(const RenderHandle& handle) const;
    RenderHandleReference GetVertexInputDeclarationHandle(const BASE_NS::string_view path) const override;
    VertexInputDeclarationView GetVertexInputDeclarationView(const RenderHandleReference& handle) const override;
    VertexInputDeclarationView GetVertexInputDeclarationView(const RenderHandle& handle) const;

    RenderHandleReference GetPipelineLayoutHandleByShaderHandle(const RenderHandleReference& handle) const override;
    RenderHandleReference GetPipelineLayoutHandleByShaderHandle(const RenderHandle& handle) const;

    RenderHandleReference GetPipelineLayoutHandle(const BASE_NS::string_view path) const override;
    PipelineLayout GetPipelineLayout(const RenderHandleReference& handle) const override;
    PipelineLayout GetPipelineLayout(const RenderHandle& handle) const;
    const PipelineLayout& GetPipelineLayoutRef(const RenderHandle& handle) const;

    RenderHandleReference GetReflectionPipelineLayoutHandle(const RenderHandle& handle) const;
    RenderHandleReference GetReflectionPipelineLayoutHandle(const RenderHandleReference& handle) const override;
    PipelineLayout GetReflectionPipelineLayout(const RenderHandleReference& handle) const override;
    const PipelineLayout& GetReflectionPipelineLayoutRef(const RenderHandle& handle) const;
    ShaderSpecializationConstantView GetReflectionSpecialization(const RenderHandleReference& handle) const override;
    ShaderSpecializationConstantView GetReflectionSpecialization(const RenderHandle& handle) const;
    VertexInputDeclarationView GetReflectionVertexInputDeclaration(const RenderHandleReference& handle) const override;
    VertexInputDeclarationView GetReflectionVertexInputDeclaration(const RenderHandle& handle) const;
    ShaderThreadGroup GetReflectionThreadGroupSize(const RenderHandleReference& handle) const override;
    ShaderThreadGroup GetReflectionThreadGroupSize(const RenderHandle& handle) const;

    uint64_t HashGraphicsState(const GraphicsState& graphicsState) const override;

    struct ShaderPathCreateData {
        const BASE_NS::string_view baseShaderPath;
        const BASE_NS::string_view variantName;
        const BASE_NS::string_view displayName;
    };
    RenderHandleReference Create(const ComputeShaderCreateData& createInfo, const ShaderPathCreateData& pathCreateInfo);
    RenderHandleReference Create(const ShaderCreateData& createInfo, const ShaderPathCreateData& pathCreateInfo);
    // NOTE: Can be used to add additional base uri name for a shader which has variant names
    void AddAdditionalNameForHandle(const RenderHandleReference& handle, const BASE_NS::string_view path);

    const GpuComputeProgram* GetGpuComputeProgram(const RenderHandle& gpuHandle) const;
    const GpuShaderProgram* GetGpuShaderProgram(const RenderHandle& gpuHandle) const;

    void HandlePendingAllocations();

    // replaces if already found
    uint32_t CreateShaderModule(const BASE_NS::string_view path, const ShaderModuleCreateInfo& createInfo);
    // returns null if not found (i.e. needs to be created)
    ShaderModule* GetShaderModule(const uint32_t index) const;
    // returns ~0u if not found (i.e. needs to be created)
    uint32_t GetShaderModuleIndex(const BASE_NS::string_view path) const;

    bool IsComputeShader(const RenderHandleReference& handle) const override;
    bool IsShader(const RenderHandleReference& handle) const override;

    void LoadShaderFiles(const ShaderFilePathDesc& desc) override;
    void LoadShaderFile(const BASE_NS::string_view uri) override;
    void ReloadShaderFile(const BASE_NS::string_view uri) override;
    void UnloadShaderFiles(const ShaderFilePathDesc& desc) override;

    const BASE_NS::string_view GetShaderFile(const RenderHandleReference& handle) const override;
    const CORE_NS::json::value* GetMaterialMetadata(const RenderHandleReference& handle) const override;

    void Destroy(const RenderHandleReference& handle) override;

    BASE_NS::vector<RenderHandleReference> GetShaders(
        const RenderHandleReference& handle, const ShaderStageFlags shaderStageFlags) const override;
    BASE_NS::vector<RenderHandle> GetShaders(const RenderHandle& handle, const ShaderStageFlags shaderStageFlags) const;

    BASE_NS::vector<RenderHandleReference> GetShaders() const override;
    BASE_NS::vector<RenderHandleReference> GetGraphicsStates() const override;
    BASE_NS::vector<RenderHandleReference> GetPipelineLayouts() const override;
    BASE_NS::vector<RenderHandleReference> GetVertexInputDeclarations() const override;

    IdDesc GetIdDesc(const RenderHandleReference& handle) const override;
    uint64_t GetFrameIndex(const RenderHandleReference& handle) const override;

    IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(const RenderHandleReference& handle) const override;
    IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(
        const RenderHandleReference& handle, const RenderHandleReference& plHandle) const override;
    IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(
        const RenderHandleReference& handle, const PipelineLayout& pipelineLayout) const override;

    CompatibilityFlags GetCompatibilityFlags(const RenderHandle& lhs, const RenderHandle& rhs) const;
    CompatibilityFlags GetCompatibilityFlags(
        const RenderHandleReference& lhs, const RenderHandleReference& rhs) const override;

    GraphicsStateFlags GetForcedGraphicsStateFlags(const RenderHandle& handle) const;
    GraphicsStateFlags GetForcedGraphicsStateFlags(const RenderHandleReference& handle) const override;
    GraphicsStateFlags GetForcedGraphicsStateFlags(const uint32_t renderSlotId) const override;

    // set (engine) file manager to be usable with shader loading with shader manager api
    void SetFileManager(CORE_NS::IFileManager& fileMgr);
    bool HasReloadedShaderForBackend() const;
    BASE_NS::array_view<const RenderHandle> GetReloadedShadersForBackend() const;

    struct ShaderNameData {
        BASE_NS::fixed_string<MAX_DEFAULT_NAME_LENGTH> path;
        BASE_NS::fixed_string<MAX_DEFAULT_NAME_LENGTH> variantName;
        BASE_NS::fixed_string<MAX_DEFAULT_NAME_LENGTH> displayName;
    };
    struct ComputeMappings {
        struct Data {
            RenderHandleReference rhr;
            RenderHandle baseShaderHandle; // provides a link to find related shaders

            uint32_t renderSlotId { INVALID_SM_INDEX };
            uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
            uint32_t reflectionPipelineLayoutIndex { INVALID_SM_INDEX };
            uint32_t categoryId { INVALID_SM_INDEX };

            uint64_t frameIndex { 0 };
        };
        BASE_NS::vector<Data> clientData;
        BASE_NS::vector<ShaderNameData> nameData;
    };
    struct GraphicsMappings {
        struct Data {
            RenderHandleReference rhr;
            RenderHandle baseShaderHandle; // provides a link to find related shaders

            uint32_t renderSlotId { INVALID_SM_INDEX };
            uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
            uint32_t reflectionPipelineLayoutIndex { INVALID_SM_INDEX };

            uint32_t vertexInputDeclarationIndex { INVALID_SM_INDEX };
            uint32_t graphicsStateIndex { INVALID_SM_INDEX };

            uint32_t categoryId { INVALID_SM_INDEX };

            uint64_t frameIndex { 0 };
        };
        BASE_NS::vector<Data> clientData;
        BASE_NS::vector<ShaderNameData> nameData;
    };
    struct GraphicsStateData {
        struct Indices {
            uint64_t hash { 0 };
            uint32_t renderSlotId { INVALID_SM_INDEX };
            uint32_t baseVariantIndex { INVALID_SM_INDEX };
            // forced state flags
            GraphicsStateFlags stateFlags { 0u };
        };
        BASE_NS::vector<RenderHandleReference> rhr;
        BASE_NS::vector<GraphicsState> graphicsStates;
        BASE_NS::vector<Indices> data;

        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToIndex;
        BASE_NS::unordered_map<uint64_t, uint32_t> hashToIndex;
        // hash based on base graphics state handle and render slot id
        BASE_NS::unordered_map<uint64_t, uint32_t> variantHashToIndex;
    };

private:
    Device& device_;
    CORE_NS::IFileManager* fileMgr_ { nullptr }; // engine file manager to be used with shader loading from API
    BASE_NS::unique_ptr<ShaderLoader> shaderLoader_;

    // for all shaders, names are unique
    BASE_NS::unordered_map<BASE_NS::string, RenderHandle> nameToClientHandle_;
    // hash based on base shader handle id and render slot id
    BASE_NS::unordered_map<uint64_t, RenderHandle> hashToShaderVariant_;
    ComputeMappings computeShaderMappings_;
    GraphicsMappings shaderMappings_;

    struct ClientDataIndices {
        uint32_t renderSlotIndex { INVALID_SM_INDEX };
        uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
        uint32_t reflectionPipelineLayoutIndex { INVALID_SM_INDEX };
        uint32_t categoryIndex { INVALID_SM_INDEX };
    };
    RenderHandle CreateClientData(
        const BASE_NS::string_view path, const RenderHandleType type, const ClientDataIndices& cdi);

    void DestroyShader(const RenderHandle handle);
    void DestroyGraphicsState(const RenderHandle handle);
    void DestroyPipelineLayout(const RenderHandle handle);
    void DestroyVertexInputDeclaration(const RenderHandle handle);

    IdDesc GetShaderIdDesc(const RenderHandle handle) const;
    uint64_t GetShaderFrameIndex(const RenderHandle handle) const;

    BASE_NS::string GetCategoryName(const uint32_t categoryId) const;

    // NOTE: ATM GpuComputeProgram and GpuShaderPrograms are currently re-created for every new shader created
    // will be stored and re-used in the future

    struct ComputeShaderData {
        BASE_NS::unique_ptr<GpuComputeProgram> gsp;
        uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
        uint32_t compModuleIndex { INVALID_SM_INDEX };
    };
    BASE_NS::vector<ComputeShaderData> computeShaders_;

    struct ShaderData {
        BASE_NS::unique_ptr<GpuShaderProgram> gsp;
        uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
        uint32_t vertexInputDeclIndex { INVALID_SM_INDEX };

        uint32_t vertModuleIndex { INVALID_SM_INDEX };
        uint32_t fragModuleIndex { INVALID_SM_INDEX };
    };
    BASE_NS::vector<ShaderData> shaders_;

    struct RenderSlotIds {
        BASE_NS::vector<RenderSlotData> data;
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToId;
    };
    RenderSlotIds renderSlotIds_;

    struct Category {
        BASE_NS::vector<BASE_NS::string> data;
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToId;
    };
    Category category_;

    struct ComputeShaderAllocs {
        RenderHandle handle;
        uint32_t computeModuleIndex { INVALID_SM_INDEX };
        uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
    };
    struct ShaderAllocs {
        RenderHandle handle;
        uint32_t vertModuleIndex { INVALID_SM_INDEX };
        uint32_t fragModuleIndex { INVALID_SM_INDEX };
        uint32_t pipelineLayoutIndex { INVALID_SM_INDEX };
        uint32_t vertexInputDeclIndex { INVALID_SM_INDEX };
    };
    struct Allocs {
        // client handle, object
        BASE_NS::vector<ComputeShaderAllocs> computeShaders;
        BASE_NS::vector<ShaderAllocs> shaders;
        BASE_NS::vector<uint32_t> recreatedComputeModuleIndices;
        BASE_NS::vector<uint32_t> recreatedShaderModuleIndices;

        BASE_NS::vector<RenderHandle> destroyHandles;
    };
    Allocs pendingAllocations_;
    // locks only pending allocations data
    std::mutex pendingMutex_;

    void HandlePendingShaders(Allocs& allocs);
    void HandlePendingModules(Allocs& allocs);

    struct DeferredDestructions {
        struct Module {
            uint64_t frameIndex { 0 };
            BASE_NS::unique_ptr<ShaderModule> shaderModule;
        };
        struct ComputeProgram {
            uint64_t frameIndex { 0 };
            BASE_NS::unique_ptr<GpuComputeProgram> computeProgram;
        };
        struct ShaderProgram {
            uint64_t frameIndex { 0 };
            BASE_NS::unique_ptr<GpuShaderProgram> shaderProgram;
        };
        BASE_NS::vector<Module> shaderModules;
        BASE_NS::vector<ComputeProgram> computePrograms;
        BASE_NS::vector<ShaderProgram> shaderPrograms;
    };
    DeferredDestructions deferredDestructions_;

    struct PL {
        BASE_NS::vector<RenderHandleReference> rhr;
        BASE_NS::vector<PipelineLayout> data;
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToIndex;

        BASE_NS::unordered_map<RenderHandle, uint32_t> computeShaderToIndex;
        BASE_NS::unordered_map<RenderHandle, uint32_t> shaderToIndex;
    };
    struct VID {
        BASE_NS::vector<RenderHandleReference> rhr;
        BASE_NS::vector<VertexInputDeclarationData> data;
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToIndex;
        BASE_NS::unordered_map<RenderHandle, uint32_t> shaderToIndex;
    };
    VID shaderVid_;
    PL pl_;

    GraphicsStateData graphicsStates_;

    struct ShaderModules {
        BASE_NS::vector<BASE_NS::unique_ptr<ShaderModule>> shaderModules;
        BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToIndex;
    };
    ShaderModules shaderModules_;

    PipelineLayout defaultPipelineLayout_;
    ComputeShaderReflection defaultComputeShaderReflection_;
    ShaderReflection defaultShaderReflection_;
    GraphicsState defaultGraphicsState_;
    ShaderSpecializationConstantView defaultSSCV_;
    VertexInputDeclarationView defaultVIDV_;
    ShaderThreadGroup defaultSTG_;

    struct MaterialMetadata {
        BASE_NS::string raw;
        CORE_NS::json::value json;
    };
    BASE_NS::unordered_map<RenderHandle, MaterialMetadata> shaderToMetadata_;
    BASE_NS::unordered_map<RenderHandle, BASE_NS::string> handleToShaderDataFile_;

    BASE_NS::vector<RenderHandle> reloadedShaders_;
    BASE_NS::vector<RenderHandle> reloadedShadersForBackend_;
};

class RenderNodeShaderManager final : public IRenderNodeShaderManager {
public:
    explicit RenderNodeShaderManager(const ShaderManager& shaderMgr);
    ~RenderNodeShaderManager() = default;

    RenderHandle GetShaderHandle(const BASE_NS::string_view path) const override;
    RenderHandle GetShaderHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const override;
    RenderHandle GetShaderHandle(const RenderHandle& name, const uint32_t renderSlotId) const override;
    BASE_NS::vector<RenderHandle> GetShaders(const uint32_t renderSlotId) const override;

    RenderHandle GetGraphicsStateHandle(const BASE_NS::string_view path) const override;
    RenderHandle GetGraphicsStateHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const override;
    RenderHandle GetGraphicsStateHandle(const RenderHandle& handle, const uint32_t renderSlotId) const override;
    RenderHandle GetGraphicsStateHandleByHash(const uint64_t hash) const override;
    RenderHandle GetGraphicsStateHandleByShaderHandle(const RenderHandle& handle) const override;
    virtual const GraphicsState& GetGraphicsState(const RenderHandle& handle) const override;

    uint32_t GetRenderSlotId(const BASE_NS::string_view renderSlot) const override;
    uint32_t GetRenderSlotId(const RenderHandle& handle) const override;
    IShaderManager::RenderSlotData GetRenderSlotData(const uint32_t renderSlotId) const override;

    RenderHandle GetVertexInputDeclarationHandleByShaderHandle(const RenderHandle& handle) const override;
    RenderHandle GetVertexInputDeclarationHandle(const BASE_NS::string_view path) const override;
    VertexInputDeclarationView GetVertexInputDeclarationView(const RenderHandle& handle) const override;

    RenderHandle GetPipelineLayoutHandleByShaderHandle(const RenderHandle& handle) const override;
    const PipelineLayout& GetPipelineLayout(const RenderHandle& handle) const override;
    RenderHandle GetPipelineLayoutHandle(const BASE_NS::string_view path) const override;

    RenderHandle GetReflectionPipelineLayoutHandle(const RenderHandle& handle) const override;
    const PipelineLayout& GetReflectionPipelineLayout(const RenderHandle& handle) const override;
    ShaderSpecializationConstantView GetReflectionSpecialization(const RenderHandle& handle) const override;
    VertexInputDeclarationView GetReflectionVertexInputDeclaration(const RenderHandle& handle) const override;
    ShaderThreadGroup GetReflectionThreadGroupSize(const RenderHandle& handle) const override;

    uint64_t HashGraphicsState(const GraphicsState& graphicsState) const override;

    bool IsValid(const RenderHandle& handle) const override;
    bool IsComputeShader(const RenderHandle& handle) const override;
    bool IsShader(const RenderHandle& handle) const override;

    BASE_NS::vector<RenderHandle> GetShaders(
        const RenderHandle& handle, const ShaderStageFlags shaderStageFlags) const override;

    IShaderManager::CompatibilityFlags GetCompatibilityFlags(
        const RenderHandle& lhs, const RenderHandle& rhs) const override;

    GraphicsStateFlags GetForcedGraphicsStateFlags(const RenderHandle& handle) const override;
    GraphicsStateFlags GetForcedGraphicsStateFlags(const uint32_t renderSlotId) const override;

private:
    const ShaderManager& shaderMgr_;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SHADER_MANAGER_H
