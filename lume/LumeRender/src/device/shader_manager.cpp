/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "shader_manager.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include <base/containers/array_view.h>
#include <core/io/intf_file_manager.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program.h"
#include "device/gpu_program_util.h"
#include "device/gpu_resource_handle_util.h"
#include "device/shader_module.h"
#include "device/shader_pipeline_binder.h"
#include "loader/shader_loader.h"
#include "resource_handle_impl.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

constexpr uint64_t IA_HASH_PRIMITIVE_TOPOLOGY_SHIFT = 1;

constexpr uint64_t RS_HASH_POLYGON_MODE_SHIFT = 4;
constexpr uint64_t RS_HASH_CULL_MODE_SHIFT = 8;
constexpr uint64_t RS_HASH_FRONT_FACE_SHIFT = 12;

constexpr uint64_t DSS_HASH_DEPTH_COMPARE_SHIFT = 4;

constexpr uint64_t HASH_RS_SHIFT = 0;
constexpr uint64_t HASH_DS_SHIFT = 32;
constexpr uint64_t HASH_IA_SHIFT = 56;

union FloatAsUint32 {
    float f;
    uint32_t ui;
};

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState::InputAssembly& inputAssembly)
{
    uint64_t hash = 0;
    hash |= static_cast<uint64_t>(inputAssembly.enablePrimitiveRestart);
    hash |= (static_cast<uint64_t>(inputAssembly.primitiveTopology) << IA_HASH_PRIMITIVE_TOPOLOGY_SHIFT);
    return hash;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState::RasterizationState& state)
{
    uint64_t hash = 0;
    hash |= (static_cast<uint64_t>(state.enableRasterizerDiscard) << 2u) |
            (static_cast<uint64_t>(state.enableDepthBias) << 1u) | static_cast<uint64_t>(state.enableDepthClamp);
    hash |= (static_cast<uint64_t>(state.polygonMode) << RS_HASH_POLYGON_MODE_SHIFT);
    hash |= (static_cast<uint64_t>(state.cullModeFlags) << RS_HASH_CULL_MODE_SHIFT);
    hash |= (static_cast<uint64_t>(state.frontFace) << RS_HASH_FRONT_FACE_SHIFT);
    return hash;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState::DepthStencilState& state)
{
    uint64_t hash = 0;
    hash |= (static_cast<uint64_t>(state.enableStencilTest) << 3u) |
            (static_cast<uint64_t>(state.enableDepthBoundsTest) << 2u) |
            (static_cast<uint64_t>(state.enableDepthWrite) << 1u) | static_cast<uint64_t>(state.enableDepthTest);
    hash |= (static_cast<uint64_t>(state.depthCompareOp) << DSS_HASH_DEPTH_COMPARE_SHIFT);
    return hash;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState::ColorBlendState::Attachment& state)
{
    uint64_t hash = 0;
    hash |= (static_cast<uint64_t>(state.enableBlend) << 0u);
    // blend factor values 0 - 18, 0x1f for exact (5 bits)
    hash |= (static_cast<uint64_t>(state.srcColorBlendFactor) << 1u);
    hash |= ((static_cast<uint64_t>(state.dstColorBlendFactor) & 0x1f) << 6u);
    hash |= ((static_cast<uint64_t>(state.srcAlphaBlendFactor) & 0x1f) << 12u);
    hash |= ((static_cast<uint64_t>(state.dstAlphaBlendFactor) & 0x1f) << 18u);
    // blend op values 0 - 4, 0x7 for exact (3 bits)
    hash |= ((static_cast<uint64_t>(state.colorBlendOp) & 0x7) << 24u);
    hash |= ((static_cast<uint64_t>(state.alphaBlendOp) & 0x7) << 28u);
    return hash;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState::ColorBlendState& state)
{
    uint64_t hash = 0;
    hash |= (static_cast<uint64_t>(state.enableLogicOp) << 0u);
    hash |= (static_cast<uint64_t>(state.logicOp) << 1u);

    FloatAsUint32 vec[4u] = { { state.colorBlendConstants[0u] }, { state.colorBlendConstants[1u] },
        { state.colorBlendConstants[2u] }, { state.colorBlendConstants[3u] } };
    const uint64_t hashRG = (static_cast<uint64_t>(vec[0u].ui) << 32) | (vec[1u].ui);
    const uint64_t hashBA = (static_cast<uint64_t>(vec[2u].ui) << 32) | (vec[3u].ui);
    HashCombine(hash, hashRG, hashBA);
    for (uint32_t idx = 0; idx < state.colorAttachmentCount; ++idx) {
        HashCombine(hash, state.colorAttachments[idx]);
    }
    return hash;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GraphicsState& state)
{
    const uint64_t iaHash = hash(state.inputAssembly);
    const uint64_t rsHash = hash(state.rasterizationState);
    const uint64_t dsHash = hash(state.depthStencilState);
    const uint64_t dynHash = state.dynamicStateFlags;
    const uint64_t cbsHash = hash(state.colorBlendState);
    uint64_t finalHash = (iaHash << HASH_IA_SHIFT) | (rsHash << HASH_RS_SHIFT) | (dsHash << HASH_DS_SHIFT);
    HashCombine(finalHash, dynHash);
    HashCombine(finalHash, cbsHash);
    return finalHash;
}

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr inline bool IsUniformBuffer(const DescriptorType descriptorType)
{
    return ((descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
            (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
}
constexpr inline bool IsStorageBuffer(const DescriptorType descriptorType)
{
    return ((descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) ||
            (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER));
}

ShaderManager::CompatibilityFlags GetPipelineLayoutCompatibilityFlags(
    const PipelineLayout& lhs, const PipelineLayout& rhs)
{
    ShaderManager::CompatibilityFlags flags = ShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT;
    for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
        const auto& lSet = lhs.descriptorSetLayouts[setIdx];
        const auto& rSet = rhs.descriptorSetLayouts[setIdx];
        if (lSet.set == rSet.set) {
            for (uint32_t lIdx = 0; lIdx < lSet.bindings.size(); ++lIdx) {
                const auto& lBind = lSet.bindings[lIdx];
                for (uint32_t rIdx = 0; rIdx < rSet.bindings.size(); ++rIdx) {
                    const auto& rBind = rSet.bindings[rIdx];
                    if (lBind.binding == rBind.binding) {
                        if ((lBind.descriptorCount != rBind.descriptorCount) ||
                            (lBind.descriptorType != rBind.descriptorType)) {
                            // re-check dynamic offsets
                            if ((IsUniformBuffer(lBind.descriptorType) != IsUniformBuffer(rBind.descriptorType)) &&
                                (IsStorageBuffer(lBind.descriptorType) != IsStorageBuffer(rBind.descriptorType))) {
                                flags = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    if (flags != 0) {
        // check for exact match
        bool isExact = true;
        for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
            const auto& lSet = lhs.descriptorSetLayouts[setIdx];
            const auto& rSet = rhs.descriptorSetLayouts[setIdx];
            if (lSet.set == rSet.set) {
                if (lSet.bindings.size() == rSet.bindings.size()) {
                    for (size_t idx = 0; idx < lSet.bindings.size(); ++idx) {
                        const int cmpRes =
                            std::memcmp(&(lSet.bindings[idx]), &(rSet.bindings[idx]), sizeof(lSet.bindings[idx]));
                        if (cmpRes != 0) {
                            isExact = false;
                            break;
                        }
                    }
                } else {
                    isExact = false;
                    break;
                }
            }
        }
        if (isExact) {
            flags |= ShaderManager::CompatibilityFlagBits::EXACT_BIT;
        }
    }
    return flags;
}

// NOTE: checking the type for validity is enough
inline bool IsComputeShaderFunc(RenderHandle handle)
{
    return RenderHandleType::COMPUTE_SHADER_STATE_OBJECT == RenderHandleUtil::GetHandleType(handle);
}

inline bool IsShaderFunc(RenderHandle handle)
{
    return RenderHandleType::SHADER_STATE_OBJECT == RenderHandleUtil::GetHandleType(handle);
}

inline bool IsAnyShaderFunc(RenderHandle handle)
{
    return (RenderHandleType::COMPUTE_SHADER_STATE_OBJECT == RenderHandleUtil::GetHandleType(handle)) ||
           (RenderHandleType::SHADER_STATE_OBJECT == RenderHandleUtil::GetHandleType(handle));
}

inline void GetShadersBySlot(
    const uint32_t renderSlotId, const ShaderManager::ComputeMappings& mappings, vector<RenderHandleReference>& shaders)
{
    for (const auto& ref : mappings.clientData) {
        if (ref.renderSlotId == renderSlotId) {
            shaders.emplace_back(ref.rhr);
        }
    }
}

inline void GetShadersBySlot(const uint32_t renderSlotId, const ShaderManager::GraphicsMappings& mappings,
    vector<RenderHandleReference>& shaders)
{
    for (const auto& ref : mappings.clientData) {
        if (ref.renderSlotId == renderSlotId) {
            shaders.emplace_back(ref.rhr);
        }
    }
}

inline void GetShadersBySlot(
    const uint32_t renderSlotId, const ShaderManager::ComputeMappings& mappings, vector<RenderHandle>& shaders)
{
    for (const auto& ref : mappings.clientData) {
        if (ref.renderSlotId == renderSlotId) {
            shaders.emplace_back(ref.rhr.GetHandle());
        }
    }
}

inline void GetShadersBySlot(
    const uint32_t renderSlotId, const ShaderManager::GraphicsMappings& mappings, vector<RenderHandle>& shaders)
{
    for (const auto& ref : mappings.clientData) {
        if (ref.renderSlotId == renderSlotId) {
            shaders.emplace_back(ref.rhr.GetHandle());
        }
    }
}

inline RenderHandle GetHandle(const string_view name, const unordered_map<string, RenderHandle>& nameToClientHandle)
{
    if (auto const pos = nameToClientHandle.find(name); pos != nameToClientHandle.end()) {
        return pos->second;
    }
    return {};
}

constexpr inline uint64_t HashHandleAndSlot(const RenderHandle& handle, const uint32_t renderSlotId)
{
    // normally there are < 16 render slot ids used which way less than 0xffff
    PLUGIN_ASSERT(renderSlotId < 0xffff);
    return (handle.id << 16ull) | (renderSlotId & 0xffff);
}

uint32_t GetBaseGraphicsStateVariantIndex(
    const ShaderManager::GraphicsStateData& graphicsStates, const ShaderManager::GraphicsStateVariantCreateInfo& vci)
{
    uint32_t baseVariantIndex = INVALID_SM_INDEX;
    if (!vci.baseShaderState.empty()) {
        const string fullBaseName = vci.baseShaderState + vci.baseVariant;
        if (const auto bhIter = graphicsStates.nameToIndex.find(fullBaseName);
            bhIter != graphicsStates.nameToIndex.cend()) {
            PLUGIN_ASSERT(bhIter->second < graphicsStates.rhr.size());
            if ((bhIter->second < graphicsStates.rhr.size()) && graphicsStates.rhr[bhIter->second]) {
                const RenderHandle baseHandle = graphicsStates.rhr[bhIter->second].GetHandle();
                baseVariantIndex = RenderHandleUtil::GetIndexPart(baseHandle);
            }
        } else {
            PLUGIN_LOG_W("base state not found (%s %s)", vci.baseShaderState.data(), vci.baseVariant.data());
        }
    }
    return baseVariantIndex;
}
} // namespace

ShaderManager::ShaderManager(Device& device) : device_(device) {}

ShaderManager::~ShaderManager() = default;

uint64_t ShaderManager::HashGraphicsState(const GraphicsState& graphicsState) const
{
    return BASE_NS::hash(graphicsState);
}

uint32_t ShaderManager::CreateRenderSlotId(const string_view renderSlot)
{
    if (renderSlot.empty()) {
        return INVALID_SM_INDEX;
    }

    if (const auto iter = renderSlotIds_.nameToId.find(renderSlot); iter != renderSlotIds_.nameToId.cend()) {
        return iter->second;
    } else { // create new id
        const uint32_t renderSlotId = static_cast<uint32_t>(renderSlotIds_.data.size());
        renderSlotIds_.nameToId[renderSlot] = renderSlotId;
        renderSlotIds_.data.push_back(RenderSlotData { renderSlotId, {}, {} });
        return renderSlotId;
    }
}

void ShaderManager::SetRenderSlotData(
    const uint32_t renderSlotId, const RenderHandleReference& shaderHandle, const RenderHandleReference& stateHandle)
{
    if (renderSlotId < static_cast<uint32_t>(renderSlotIds_.data.size())) {
        if (IsAnyShaderFunc(shaderHandle.GetHandle())) {
            renderSlotIds_.data[renderSlotId].shader = shaderHandle;
        }
        if (RenderHandleUtil::GetHandleType(stateHandle.GetHandle()) == RenderHandleType::GRAPHICS_STATE) {
            renderSlotIds_.data[renderSlotId].graphicsState = stateHandle;
        }
    }
}

RenderHandle ShaderManager::CreateClientData(
    const string_view name, const RenderHandleType type, const ClientDataIndices& cdi)
{
    RenderHandle clientHandle;
    PLUGIN_ASSERT(
        (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) || (type == RenderHandleType::SHADER_STATE_OBJECT));
    if (const auto iter = nameToClientHandle_.find(name); iter != nameToClientHandle_.end()) {
        clientHandle = iter->second;
    } else {
        const uint32_t arrayIndex = (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT)
                                        ? static_cast<uint32_t>(computeShaderMappings_.clientData.size())
                                        : static_cast<uint32_t>(shaderMappings_.clientData.size());
        clientHandle = RenderHandleUtil::CreateGpuResourceHandle(type, 0, arrayIndex, 0);
        RenderHandleReference rhr =
            RenderHandleReference(clientHandle, IRenderReferenceCounter::Ptr(new ShaderReferenceCounter()));
        if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
            computeShaderMappings_.clientData.push_back(
                { move(rhr), {}, cdi.renderSlotIndex, cdi.pipelineLayoutIndex, cdi.reflectionPipelineLayoutIndex });
        } else {
            shaderMappings_.clientData.push_back(
                { move(rhr), {}, cdi.renderSlotIndex, cdi.pipelineLayoutIndex, cdi.reflectionPipelineLayoutIndex });
        }
        if (!name.empty()) {
            nameToClientHandle_[name] = clientHandle;
        }
    }

    return clientHandle;
}

RenderHandleReference ShaderManager::Create(
    const ComputeShaderCreateData& createInfo, const string_view baseShaderPath, const string_view variantName)
{
    const string fullName = createInfo.path + variantName;

    // reflection pipeline layout
    uint32_t reflectionPlIndex = INVALID_SM_INDEX;
    if (const ShaderModule* cs = GetShaderModule(createInfo.shaderModuleIndex); cs) {
        const RenderHandleReference plRhr = CreatePipelineLayout({ fullName, cs->GetPipelineLayout() });
        reflectionPlIndex = RenderHandleUtil::GetIndexPart(plRhr.GetHandle());
    }

    auto const clientHandle = CreateClientData(fullName, RenderHandleType::COMPUTE_SHADER_STATE_OBJECT,
        { createInfo.renderSlotId, createInfo.pipelineLayoutIndex, reflectionPlIndex });
    if (createInfo.pipelineLayoutIndex != INVALID_SM_INDEX) {
        pl_.computeShaderToIndex[clientHandle] = createInfo.pipelineLayoutIndex;
    }

    {
        const auto lock = std::lock_guard(pendingMutex_);
        pendingAllocations_.computeShaders.push_back(
            { clientHandle, createInfo.shaderModuleIndex, createInfo.pipelineLayoutIndex });
    }

    const uint32_t index = RenderHandleUtil::GetIndexPart(clientHandle);
    if (IsComputeShaderFunc(clientHandle) &&
        (index < static_cast<uint32_t>(computeShaderMappings_.clientData.size()))) {
        auto& clientDataRef = computeShaderMappings_.clientData[index];
        // add base shader if given
        if (!baseShaderPath.empty()) {
            if (const auto baseHandleIter = nameToClientHandle_.find(baseShaderPath);
                baseHandleIter != nameToClientHandle_.cend()) {
                if (RenderHandleUtil::IsValid(baseHandleIter->second)) {
                    clientDataRef.baseShaderHandle = baseHandleIter->second;
                    const uint64_t hash = HashHandleAndSlot(clientDataRef.baseShaderHandle, createInfo.renderSlotId);
                    hashToShaderVariant_[hash] = clientHandle;
                }
            } else {
                PLUGIN_LOG_W("base shader (%s) not found for (%s)", baseShaderPath.data(), createInfo.path.data());
            }
        }
        return clientDataRef.rhr;
    } else {
        return {};
    }
}

RenderHandleReference ShaderManager::Create(
    const ShaderCreateData& createInfo, const string_view baseShaderPath, const string_view variantName)
{
    const string fullName = createInfo.path + variantName;

    // reflection pipeline layout
    uint32_t reflectionPlIndex = INVALID_SM_INDEX;
    {
        const ShaderModule* vs = GetShaderModule(createInfo.vertShaderModuleIndex);
        const ShaderModule* fs = GetShaderModule(createInfo.fragShaderModuleIndex);
        if (vs && fs) {
            const PipelineLayout layouts[] { vs->GetPipelineLayout(), fs->GetPipelineLayout() };
            PipelineLayout pl;
            GpuProgramUtil::CombinePipelineLayouts({ layouts, 2u }, pl);
            const RenderHandleReference plRhr = CreatePipelineLayout({ fullName, pl });
            reflectionPlIndex = RenderHandleUtil::GetIndexPart(plRhr.GetHandle());
        }
    }

    auto const clientHandle = CreateClientData(fullName, RenderHandleType::SHADER_STATE_OBJECT,
        { createInfo.renderSlotId, createInfo.pipelineLayoutIndex, reflectionPlIndex });

    if (createInfo.pipelineLayoutIndex != INVALID_SM_INDEX) {
        pl_.shaderToIndex[clientHandle] = createInfo.pipelineLayoutIndex;
    }
    if (createInfo.vertexInputDeclarationIndex != INVALID_SM_INDEX) {
        shaderVid_.shaderToIndex[clientHandle] = createInfo.vertexInputDeclarationIndex;
    }

    {
        const auto lock = std::lock_guard(pendingMutex_);
        pendingAllocations_.shaders.push_back({ clientHandle, createInfo.vertShaderModuleIndex,
            createInfo.fragShaderModuleIndex, createInfo.pipelineLayoutIndex, createInfo.vertexInputDeclarationIndex });
    }

    if (!createInfo.materialMetadata.empty()) {
        MaterialMetadata metadata { string(createInfo.materialMetadata), json::value {} };
        metadata.json = json::parse(metadata.raw.data());
        if (metadata.json) {
            shaderToMetadata_.insert({ clientHandle, move(metadata) });
        }
    }

    const uint32_t index = RenderHandleUtil::GetIndexPart(clientHandle);
    if (IsShaderFunc(clientHandle) && (index < static_cast<uint32_t>(shaderMappings_.clientData.size()))) {
        auto& clientDataRef = shaderMappings_.clientData[index];
        clientDataRef.graphicsStateIndex = createInfo.graphicsStateIndex;
        clientDataRef.vertexInputDeclarationIndex = createInfo.vertexInputDeclarationIndex;
        // add base shader if given
#if (RENDER_VALIDATION_ENABLED == 1)
        if ((!variantName.empty()) && baseShaderPath.empty()) {
            PLUGIN_LOG_W("RENDER_VALIDATION: base shader path not give to variant (%s %s)", createInfo.path.data(),
                variantName.data());
        }
#endif
        if (!baseShaderPath.empty()) {
            if (const auto baseHandleIter = nameToClientHandle_.find(baseShaderPath);
                baseHandleIter != nameToClientHandle_.cend()) {
                if (RenderHandleUtil::IsValid(baseHandleIter->second)) {
                    clientDataRef.baseShaderHandle = baseHandleIter->second;
                    const uint64_t hash = HashHandleAndSlot(clientDataRef.baseShaderHandle, createInfo.renderSlotId);
                    hashToShaderVariant_[hash] = clientHandle;
                }
            } else {
                PLUGIN_LOG_W("base shader (%s) not found for (%s)", baseShaderPath.data(), createInfo.path.data());
            }
        }
        return clientDataRef.rhr;
    } else {
        return {};
    }
}

void ShaderManager::AddAdditionalNameForHandle(const RenderHandleReference& handle, const string_view name)
{
    if (handle) {
        const RenderHandle rawHandle = handle.GetHandle();
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(rawHandle);
        // add name only if name not used yet
        if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) ||
            (handleType == RenderHandleType::SHADER_STATE_OBJECT)) {
            if (!nameToClientHandle_.contains(name)) {
                nameToClientHandle_[name] = rawHandle;
            } else {
                PLUGIN_LOG_W("trying to add additional name (%s) for shader handle, but the name is already in use",
                    name.data());
            }
        }
    }
}

RenderHandleReference ShaderManager::CreateComputeShader(
    const ComputeShaderCreateInfo& createInfo, const string_view baseShaderPath, const string_view variantName)
{
    if (createInfo.shaderPaths.size() >= 1u) {
        if (const uint32_t moduleIdx = GetShaderModuleIndex(createInfo.shaderPaths[0].path);
            moduleIdx != INVALID_SM_INDEX) {
            return Create(ComputeShaderCreateData { createInfo.path, createInfo.renderSlotId,
                              RenderHandleUtil::GetIndexPart(createInfo.pipelineLayout), moduleIdx },
                baseShaderPath, variantName);
        } else {
            PLUGIN_LOG_E("ShaderManager: compute shader (%s) creation failed, compute shader path (%s) not found",
                string(createInfo.path).c_str(), string(createInfo.shaderPaths[0].path).c_str());
        }
    } else {
        PLUGIN_LOG_E("ShaderManager: compute shader (%s) creation failed, no shader module paths given",
            string(createInfo.path).c_str());
    }
    return {};
}

RenderHandleReference ShaderManager::CreateComputeShader(const ComputeShaderCreateInfo& createInfo)
{
    return CreateComputeShader(createInfo, "", "");
}

RenderHandleReference ShaderManager::CreateShader(
    const ShaderCreateInfo& createInfo, const string_view baseShaderPath, const string_view variantName)
{
    if (createInfo.shaderPaths.size() >= 2u) {
        const uint32_t vertShaderModule = GetShaderModuleIndex(createInfo.shaderPaths[0u].path);
        const uint32_t fragShaderModule = GetShaderModuleIndex(createInfo.shaderPaths[1u].path);
        if ((vertShaderModule != INVALID_SM_INDEX) && (fragShaderModule != INVALID_SM_INDEX)) {
            return Create(
                ShaderCreateData { createInfo.path, createInfo.renderSlotId,
                    RenderHandleUtil::GetIndexPart(createInfo.vertexInputDeclaration),
                    RenderHandleUtil::GetIndexPart(createInfo.pipelineLayout),
                    RenderHandleUtil::GetIndexPart(createInfo.graphicsState), vertShaderModule, fragShaderModule, {} },
                baseShaderPath, variantName);
        } else {
            PLUGIN_LOG_E("ShaderManager: shader (%s) creation failed, shader path (vert:%s) (frag:%s) not found",
                string(createInfo.path).c_str(), string(createInfo.shaderPaths[0u].path).c_str(),
                string(createInfo.shaderPaths[1u].path).c_str());
        }
    } else {
        PLUGIN_LOG_E("ShaderManager: shader (%s) creation failed, no shader module paths given",
            string(createInfo.path).c_str());
    }
    return {};
}

RenderHandleReference ShaderManager::CreateShader(const ShaderCreateInfo& createInfo)
{
    return CreateShader(createInfo, "", "");
}

void ShaderManager::HandlePendingAllocations()
{
    pendingMutex_.lock();
    decltype(pendingAllocations_) pendingAllocations = move(pendingAllocations_);
    pendingMutex_.unlock();

    for (const auto& handleRef : pendingAllocations.destroyHandles) {
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handleRef);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handleRef);
        if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
            if (arrayIndex < static_cast<uint32_t>(computeShaders_.size())) {
                computeShaders_[arrayIndex] = {};
            }
        } else if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
            if (arrayIndex < static_cast<uint32_t>(shaders_.size())) {
                shaders_[arrayIndex] = {};
            }
        }
    }
    HandlePendingShaders(pendingAllocations);
    HandlePendingModules(pendingAllocations);

    const uint64_t frameCount = device_.GetFrameCount();
    constexpr uint64_t additionalFrameCount { 2u };
    const auto minAge = device_.GetCommandBufferingCount() + additionalFrameCount;
    const auto ageLimit = (frameCount < minAge) ? 0 : (frameCount - minAge);
    auto CompareForErase = [](const auto ageLimit, auto& vec) {
        for (auto iter = vec.begin(); iter != vec.end();) {
            if (iter->frameIndex < ageLimit) {
                iter = vec.erase(iter);
            } else {
                ++iter;
            }
        }
    };
    CompareForErase(ageLimit, deferredDestructions_.shaderModules);
    CompareForErase(ageLimit, deferredDestructions_.computePrograms);
    CompareForErase(ageLimit, deferredDestructions_.shaderPrograms);

    hasReloadedShaders_ = false;
}

void ShaderManager::HandlePendingShaders(Allocs& allocs)
{
    const uint64_t frameCount = device_.GetFrameCount();
    for (const auto& ref : allocs.computeShaders) {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(ref.handle);
        ShaderModule* shaderModule = GetShaderModule(ref.computeModuleIndex);
        if (shaderModule) {
            if (arrayIndex < static_cast<uint32_t>(computeShaders_.size())) {
                // replace with new (push old for deferred destruction)
                deferredDestructions_.computePrograms.push_back({ frameCount, move(computeShaders_[arrayIndex].gsp) });
                computeShaders_[arrayIndex] = { device_.CreateGpuComputeProgram({ shaderModule }),
                    ref.pipelineLayoutIndex, ref.computeModuleIndex };
            } else {
                // new gpu resource
                computeShaders_.push_back({ device_.CreateGpuComputeProgram({ shaderModule }), ref.pipelineLayoutIndex,
                    ref.computeModuleIndex });
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (!shaderModule) {
            PLUGIN_LOG_E("RENDER_VALIDATION: Compute shader module with index:%u, not found", ref.computeModuleIndex);
        }
#endif
    }
    for (const auto& ref : allocs.shaders) {
        uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(ref.handle);
        ShaderModule* vertShaderModule = GetShaderModule(ref.vertModuleIndex);
        ShaderModule* fragShaderModule = GetShaderModule(ref.fragModuleIndex);
        if (vertShaderModule && fragShaderModule) {
            if ((arrayIndex < static_cast<uint32_t>(shaders_.size()))) {
                // replace with new (push old for deferred destruction)
                deferredDestructions_.shaderPrograms.push_back({ frameCount, move(shaders_[arrayIndex].gsp) });
                shaders_[arrayIndex] = { device_.CreateGpuShaderProgram({ vertShaderModule, fragShaderModule }),
                    ref.pipelineLayoutIndex, ref.vertexInputDeclIndex, ref.vertModuleIndex, ref.fragModuleIndex };
            } else { // new gpu resource
                shaders_.push_back({ device_.CreateGpuShaderProgram({ vertShaderModule, fragShaderModule }),
                    ref.pipelineLayoutIndex, ref.vertexInputDeclIndex, ref.vertModuleIndex, ref.fragModuleIndex });
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if ((!vertShaderModule) || (!fragShaderModule)) {
            PLUGIN_LOG_E("RENDER_VALIDATION: Shader module with index: %u or %u, not found", ref.vertModuleIndex,
                ref.fragModuleIndex);
        }
#endif
    }
}

void ShaderManager::HandlePendingModules(Allocs& allocs)
{
    const uint64_t frameCount = device_.GetFrameCount();
    for (const auto modIdx : allocs.recreatedComputeModuleIndices) {
        for (auto& shaderRef : computeShaders_) {
            if (modIdx == shaderRef.compModuleIndex) {
                if (ShaderModule* compModule = GetShaderModule(shaderRef.compModuleIndex); compModule) {
                    deferredDestructions_.computePrograms.push_back({ frameCount, move(shaderRef.gsp) });
                    shaderRef.gsp = device_.CreateGpuComputeProgram({ compModule });
                }
            }
        }
    }
    for (const auto modIdx : allocs.recreatedShaderModuleIndices) {
        for (auto& shaderRef : shaders_) {
            if ((modIdx == shaderRef.vertModuleIndex) || (modIdx == shaderRef.fragModuleIndex)) {
                ShaderModule* vertModule = GetShaderModule(shaderRef.vertModuleIndex);
                ShaderModule* fragModule = GetShaderModule(shaderRef.fragModuleIndex);
                if (vertModule && fragModule) {
                    deferredDestructions_.shaderPrograms.push_back({ frameCount, move(shaderRef.gsp) });
                    shaderRef.gsp = device_.CreateGpuShaderProgram({ vertModule, fragModule });
                }
            }
        }
    }
}

RenderHandleReference ShaderManager::GetShaderHandle(const string_view name) const
{
    const RenderHandle handle = GetHandle(name, nameToClientHandle_);
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
        (index < static_cast<uint32_t>(computeShaderMappings_.clientData.size()))) {
        return computeShaderMappings_.clientData[index].rhr;
    } else if ((handleType == RenderHandleType::SHADER_STATE_OBJECT) &&
               (index < static_cast<uint32_t>(shaderMappings_.clientData.size()))) {
        return shaderMappings_.clientData[index].rhr;
    } else {
        PLUGIN_LOG_W("ShaderManager: invalid shader %s", name.data());
        return {};
    }
}

RenderHandleReference ShaderManager::GetShaderHandle(const string_view name, const string_view variantName) const
{
    const string fullName = name + variantName;
    const RenderHandle handle = GetHandle(name, nameToClientHandle_);
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
        (index < static_cast<uint32_t>(computeShaderMappings_.clientData.size()))) {
        return computeShaderMappings_.clientData[index].rhr;
    } else if ((handleType == RenderHandleType::SHADER_STATE_OBJECT) &&
               (index < static_cast<uint32_t>(shaderMappings_.clientData.size()))) {
        return shaderMappings_.clientData[index].rhr;
    } else {
        PLUGIN_LOG_W("ShaderManager: invalid shader (%s) variant (%s)", name.data(), variantName.data());
        return {};
    }
}

RenderHandleReference ShaderManager::GetShaderHandle(const RenderHandle& handle, const uint32_t renderSlotId) const
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    if ((handleType != RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
        (handleType != RenderHandleType::SHADER_STATE_OBJECT)) {
        return {}; // early out
    }

    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    RenderHandle baseShaderHandle;
    // check first for own validity and possible base shader handle
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
        (index < static_cast<uint32_t>(computeShaderMappings_.clientData.size()))) {
        const auto& ref = computeShaderMappings_.clientData[index];
        if (ref.renderSlotId == renderSlotId) {
            return ref.rhr;
        }
        baseShaderHandle = ref.baseShaderHandle;
    } else if ((handleType == RenderHandleType::SHADER_STATE_OBJECT) &&
               (index < static_cast<uint32_t>(shaderMappings_.clientData.size()))) {
        const auto& ref = shaderMappings_.clientData[index];
        if (ref.renderSlotId == renderSlotId) {
            return ref.rhr;
        }
        baseShaderHandle = ref.baseShaderHandle;
    }
    // try to find a match through base shader variant
    if (RenderHandleUtil::IsValid(baseShaderHandle)) {
        const uint64_t hash = HashHandleAndSlot(baseShaderHandle, renderSlotId);
        if (const auto iter = hashToShaderVariant_.find(hash); iter != hashToShaderVariant_.cend()) {
            const RenderHandleType baseHandleType = RenderHandleUtil::GetHandleType(iter->second);
            const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(iter->second);
            if ((baseHandleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
                (arrayIndex < computeShaderMappings_.clientData.size())) {
                PLUGIN_ASSERT(computeShaderMappings_.clientData[arrayIndex].renderSlotId == renderSlotId);
                return computeShaderMappings_.clientData[arrayIndex].rhr;
            } else if ((baseHandleType == RenderHandleType::SHADER_STATE_OBJECT) &&
                       (arrayIndex < shaderMappings_.clientData.size())) {
                PLUGIN_ASSERT(shaderMappings_.clientData[arrayIndex].renderSlotId == renderSlotId);
                return shaderMappings_.clientData[arrayIndex].rhr;
            }
        }
    }
    return {};
}

RenderHandleReference ShaderManager::GetShaderHandle(
    const RenderHandleReference& handle, const uint32_t renderSlotId) const
{
    return GetShaderHandle(handle.GetHandle(), renderSlotId);
}

vector<RenderHandleReference> ShaderManager::GetShaders(const uint32_t renderSlotId) const
{
    vector<RenderHandleReference> shaders;
    GetShadersBySlot(renderSlotId, shaderMappings_, shaders);
    GetShadersBySlot(renderSlotId, computeShaderMappings_, shaders);
    return shaders;
}

vector<RenderHandle> ShaderManager::GetShaderRawHandles(const uint32_t renderSlotId) const
{
    vector<RenderHandle> shaders;
    GetShadersBySlot(renderSlotId, shaderMappings_, shaders);
    GetShadersBySlot(renderSlotId, computeShaderMappings_, shaders);
    return shaders;
}

RenderHandleReference ShaderManager::CreateGraphicsState(
    const GraphicsStateCreateInfo& createInfo, const GraphicsStateVariantCreateInfo& variantCreateInfo)
{
    PLUGIN_ASSERT(graphicsStates_.rhr.size() == graphicsStates_.graphicsStates.size());
    const uint32_t renderSlotId = CreateRenderSlotId(variantCreateInfo.renderSlot);
    // NOTE: No collisions expected if path is used
    const string fullName = createInfo.path + variantCreateInfo.variant;
    uint32_t arrayIndex = INVALID_SM_INDEX;
    if (auto nameIter = graphicsStates_.nameToIndex.find(fullName); nameIter != graphicsStates_.nameToIndex.end()) {
        arrayIndex = static_cast<uint32_t>(nameIter->second);
    }

    uint32_t baseVariantIndex = INVALID_SM_INDEX;
    RenderHandleReference rhr;
    if (arrayIndex < graphicsStates_.rhr.size()) {
        rhr = graphicsStates_.rhr[arrayIndex];
        graphicsStates_.graphicsStates[arrayIndex] = createInfo.graphicsState;
        const uint64_t hash = HashGraphicsState(createInfo.graphicsState);
        baseVariantIndex = GetBaseGraphicsStateVariantIndex(graphicsStates_, variantCreateInfo);
        graphicsStates_.data[arrayIndex] = { hash, renderSlotId, baseVariantIndex };
        graphicsStates_.hashToIndex[hash] = arrayIndex;
    } else { // new
        arrayIndex = static_cast<uint32_t>(graphicsStates_.rhr.size());
        // NOTE: these are only updated for new states
        if (!fullName.empty()) {
            graphicsStates_.nameToIndex[fullName] = arrayIndex;
        }
        const RenderHandle handle = RenderHandleUtil::CreateHandle(RenderHandleType::GRAPHICS_STATE, arrayIndex);
        graphicsStates_.rhr.emplace_back(
            RenderHandleReference(handle, IRenderReferenceCounter::Ptr(new ShaderReferenceCounter())));
        rhr = graphicsStates_.rhr[arrayIndex];
        graphicsStates_.graphicsStates.emplace_back(createInfo.graphicsState);
        const uint64_t hash = HashGraphicsState(createInfo.graphicsState);
        // ordering matters, this fetches from nameToIndex
        baseVariantIndex = GetBaseGraphicsStateVariantIndex(graphicsStates_, variantCreateInfo);
        graphicsStates_.data.push_back({ hash, renderSlotId, baseVariantIndex });
        graphicsStates_.hashToIndex[hash] = arrayIndex;
    }
    if (baseVariantIndex < graphicsStates_.rhr.size()) {
        const uint64_t variantHash = HashHandleAndSlot(graphicsStates_.rhr[baseVariantIndex].GetHandle(), renderSlotId);
        if (variantHash != INVALID_SM_INDEX) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (graphicsStates_.variantHashToIndex.contains(variantHash)) {
                PLUGIN_LOG_W("RENDER_VALIDATION: overwriting variant hash with %s %s", createInfo.path.data(),
                    variantCreateInfo.variant.data());
            }
#endif
            graphicsStates_.variantHashToIndex[variantHash] = RenderHandleUtil::GetIndexPart(rhr.GetHandle());
        }
    }

    return rhr;
}

RenderHandleReference ShaderManager::CreateGraphicsState(const GraphicsStateCreateInfo& createInfo)
{
    return CreateGraphicsState(createInfo, {});
}

RenderHandleReference ShaderManager::GetGraphicsStateHandle(const string_view name) const
{
    if (const auto iter = graphicsStates_.nameToIndex.find(name); iter != graphicsStates_.nameToIndex.cend()) {
        PLUGIN_ASSERT(iter->second < graphicsStates_.rhr.size());
        return graphicsStates_.rhr[iter->second];
    } else {
        PLUGIN_LOG_W("ShaderManager: named graphics state not found: %s", string(name).c_str());
        return {};
    }
}

RenderHandleReference ShaderManager::GetGraphicsStateHandle(const string_view name, const string_view variantName) const
{
    // NOTE: does not call the base GetGraphicsStateHandle due to better error logging
    const string fullName = string(name + variantName);
    if (const auto iter = graphicsStates_.nameToIndex.find(fullName); iter != graphicsStates_.nameToIndex.cend()) {
        PLUGIN_ASSERT(iter->second < graphicsStates_.rhr.size());
        return graphicsStates_.rhr[iter->second];
    } else {
        PLUGIN_LOG_W(
            "ShaderManager: named graphics state not found (name: %s variant: %s)", name.data(), variantName.data());
        return {};
    }
}

RenderHandleReference ShaderManager::GetGraphicsStateHandle(
    const RenderHandle& handle, const uint32_t renderSlotId) const
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GRAPHICS_STATE) {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        if (arrayIndex < static_cast<uint32_t>(graphicsStates_.data.size())) {
            // check for own validity
            const auto& data = graphicsStates_.data[arrayIndex];
            if (renderSlotId == data.renderSlotId) {
                return graphicsStates_.rhr[arrayIndex];
            }
            // check for base variant for hashing
            if (data.baseVariantIndex < static_cast<uint32_t>(graphicsStates_.data.size())) {
                const RenderHandle baseHandle = graphicsStates_.rhr[data.baseVariantIndex].GetHandle();
                const uint64_t hash = HashHandleAndSlot(baseHandle, renderSlotId);
                if (const auto iter = graphicsStates_.variantHashToIndex.find(hash);
                    iter != graphicsStates_.variantHashToIndex.cend()) {
                    PLUGIN_ASSERT(iter->second < static_cast<uint32_t>(graphicsStates_.rhr.size()));
                    return graphicsStates_.rhr[iter->second];
                }
            }
        }
    }
    return {};
}

RenderHandleReference ShaderManager::GetGraphicsStateHandle(
    const RenderHandleReference& handle, const uint32_t renderSlotId) const
{
    return GetGraphicsStateHandle(handle.GetHandle(), renderSlotId);
}

RenderHandleReference ShaderManager::GetGraphicsStateHandleByHash(const uint64_t hash) const
{
    if (const auto iter = graphicsStates_.hashToIndex.find(hash); iter != graphicsStates_.hashToIndex.cend()) {
        PLUGIN_ASSERT(iter->second < graphicsStates_.rhr.size());
        return graphicsStates_.rhr[iter->second];
    } else {
        return {};
    }
}

RenderHandleReference ShaderManager::GetGraphicsStateHandleByShaderHandle(const RenderHandle& handle) const
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::SHADER_STATE_OBJECT) {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        if (arrayIndex < static_cast<uint32_t>(shaderMappings_.clientData.size())) {
            const uint32_t gsIndex = shaderMappings_.clientData[arrayIndex].graphicsStateIndex;
            if (gsIndex < static_cast<uint32_t>(graphicsStates_.graphicsStates.size())) {
                return graphicsStates_.rhr[gsIndex];
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_ASSERT(gsIndex != INVALID_SM_INDEX); // not and optional index ATM
            PLUGIN_ASSERT(gsIndex < graphicsStates_.rhr.size());
#endif
        }
    }
    return {};
}

RenderHandleReference ShaderManager::GetGraphicsStateHandleByShaderHandle(const RenderHandleReference& handle) const
{
    return GetGraphicsStateHandleByShaderHandle(handle.GetHandle());
}

GraphicsState ShaderManager::GetGraphicsState(const RenderHandleReference& handle) const
{
    return GetGraphicsStateRef(handle);
}

const GraphicsState& ShaderManager::GetGraphicsStateRef(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if ((type == RenderHandleType::GRAPHICS_STATE) &&
        (arrayIndex < static_cast<uint32_t>(graphicsStates_.graphicsStates.size()))) {
        return graphicsStates_.graphicsStates[arrayIndex];
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (RenderHandleUtil::IsValid(handle) && (type != RenderHandleType::GRAPHICS_STATE)) {
            PLUGIN_LOG_W("RENDER_VALIDATION: invalid handle type given to GetGraphicsState()");
        }
#endif
        return defaultGraphicsState_;
    }
}

const GraphicsState& ShaderManager::GetGraphicsStateRef(const RenderHandleReference& handle) const
{
    return GetGraphicsStateRef(handle.GetHandle());
}

uint32_t ShaderManager::GetRenderSlotId(const string_view renderSlot) const
{
    if (const auto iter = renderSlotIds_.nameToId.find(renderSlot); iter != renderSlotIds_.nameToId.cend()) {
        return iter->second;
    } else {
        return INVALID_SM_INDEX;
    }
}

uint32_t ShaderManager::GetRenderSlotId(const RenderHandle& handle) const
{
    uint32_t id = ~0u;
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        if (arrayIndex < computeShaderMappings_.clientData.size()) {
            id = computeShaderMappings_.clientData[arrayIndex].renderSlotId;
        }
    } else if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
        if (arrayIndex < shaderMappings_.clientData.size()) {
            id = shaderMappings_.clientData[arrayIndex].renderSlotId;
        }
    } else if (handleType == RenderHandleType::GRAPHICS_STATE) {
        if (arrayIndex < graphicsStates_.data.size()) {
            id = graphicsStates_.data[arrayIndex].renderSlotId;
        }
    }
    return id;
}

uint32_t ShaderManager::GetRenderSlotId(const RenderHandleReference& handle) const
{
    return GetRenderSlotId(handle.GetHandle());
}

IShaderManager::RenderSlotData ShaderManager::GetRenderSlotData(const uint32_t renderSlotId) const
{
    if (renderSlotId < static_cast<uint32_t>(renderSlotIds_.data.size())) {
        return renderSlotIds_.data[renderSlotId];
    } else {
        return {};
    }
}

RenderHandleReference ShaderManager::GetVertexInputDeclarationHandleByShaderHandle(const RenderHandle& handle) const
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::SHADER_STATE_OBJECT) {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        auto& mappings = shaderMappings_;
        if (arrayIndex < mappings.clientData.size()) {
            const uint32_t vidIndex = mappings.clientData[arrayIndex].vertexInputDeclarationIndex;
            if (vidIndex < shaderVid_.rhr.size()) {
                PLUGIN_ASSERT(vidIndex < shaderVid_.rhr.size());
                return shaderVid_.rhr[vidIndex];
            }
        }
    }
    return {};
}

RenderHandleReference ShaderManager::GetVertexInputDeclarationHandleByShaderHandle(
    const RenderHandleReference& handle) const
{
    return GetVertexInputDeclarationHandleByShaderHandle(handle.GetHandle());
}

RenderHandleReference ShaderManager::GetVertexInputDeclarationHandle(const string_view name) const
{
    if (const auto iter = shaderVid_.nameToIndex.find(name); iter != shaderVid_.nameToIndex.cend()) {
        PLUGIN_ASSERT(iter->second < shaderVid_.rhr.size());
        return shaderVid_.rhr[iter->second];
    } else {
        PLUGIN_LOG_W("ShaderManager: named vertex input declaration not found: %s", name.data());
        return {};
    }
}

VertexInputDeclarationView ShaderManager::GetVertexInputDeclarationView(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if ((type == RenderHandleType::VERTEX_INPUT_DECLARATION) &&
        (index < static_cast<uint32_t>(shaderVid_.data.size()))) {
        const auto& ref = shaderVid_.data[index];
        return {
            array_view<const VertexInputDeclaration::VertexInputBindingDescription>(
                ref.bindingDescriptions, ref.bindingDescriptionCount),
            array_view<const VertexInputDeclaration::VertexInputAttributeDescription>(
                ref.attributeDescriptions, ref.attributeDescriptionCount),
        };
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (RenderHandleUtil::IsValid(handle) && (type != RenderHandleType::VERTEX_INPUT_DECLARATION)) {
            PLUGIN_LOG_W("RENDER_VALIDATION: invalid handle type given to GetVertexInputDeclarationView()");
        }
#endif
        return {};
    }
}

VertexInputDeclarationView ShaderManager::GetVertexInputDeclarationView(const RenderHandleReference& handle) const
{
    return GetVertexInputDeclarationView(handle.GetHandle());
}

RenderHandleReference ShaderManager::CreateVertexInputDeclaration(const VertexInputDeclarationCreateInfo& createInfo)
{
    uint32_t arrayIndex = INVALID_SM_INDEX;
    if (auto nameIter = shaderVid_.nameToIndex.find(createInfo.path); nameIter != shaderVid_.nameToIndex.end()) {
        PLUGIN_ASSERT(nameIter->second < shaderVid_.rhr.size());
        arrayIndex = static_cast<uint32_t>(nameIter->second);
    }
    if (arrayIndex < static_cast<uint32_t>(shaderVid_.data.size())) {
        // inside core validation due to being very low info for common users
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_I("ShaderManager: re-creating vertex input declaration (name %s)", createInfo.path.data());
#endif
    } else { // new
        arrayIndex = static_cast<uint32_t>(shaderVid_.data.size());
        const RenderHandle handle =
            RenderHandleUtil::CreateHandle(RenderHandleType::VERTEX_INPUT_DECLARATION, arrayIndex);
        shaderVid_.rhr.emplace_back(
            RenderHandleReference(handle, IRenderReferenceCounter::Ptr(new ShaderReferenceCounter())));
        shaderVid_.data.emplace_back(VertexInputDeclarationData {});
        // NOTE: only updated for new
        if (!createInfo.path.empty()) {
            shaderVid_.nameToIndex[createInfo.path] = arrayIndex;
        }
    }

    if (arrayIndex < static_cast<uint32_t>(shaderVid_.data.size())) {
        const VertexInputDeclarationView& vertexInputDeclarationView = createInfo.vertexInputDeclarationView;
        VertexInputDeclarationData& ref = shaderVid_.data[arrayIndex];
        ref.bindingDescriptionCount = (uint32_t)vertexInputDeclarationView.bindingDescriptions.size();
        ref.attributeDescriptionCount = (uint32_t)vertexInputDeclarationView.attributeDescriptions.size();

        PLUGIN_ASSERT(ref.bindingDescriptionCount <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
        PLUGIN_ASSERT(ref.attributeDescriptionCount <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);

        for (uint32_t idx = 0; idx < ref.bindingDescriptionCount; ++idx) {
            ref.bindingDescriptions[idx] = vertexInputDeclarationView.bindingDescriptions[idx];
        }
        for (uint32_t idx = 0; idx < ref.attributeDescriptionCount; ++idx) {
            ref.attributeDescriptions[idx] = vertexInputDeclarationView.attributeDescriptions[idx];
        }
        return shaderVid_.rhr[arrayIndex];
    } else {
        return {};
    }
}

RenderHandleReference ShaderManager::GetPipelineLayoutHandleByShaderHandle(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (type == RenderHandleType::SHADER_STATE_OBJECT) {
        auto& mappings = shaderMappings_;
        if (arrayIndex < mappings.clientData.size()) {
            const uint32_t plIndex = mappings.clientData[arrayIndex].pipelineLayoutIndex;
            if (plIndex < static_cast<uint32_t>(pl_.rhr.size())) {
                PLUGIN_ASSERT(plIndex < static_cast<uint32_t>(pl_.rhr.size()));
                return pl_.rhr[plIndex];
            }
        }
    } else if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        auto& mappings = computeShaderMappings_;
        if (arrayIndex < mappings.clientData.size()) {
            const uint32_t plIndex = mappings.clientData[arrayIndex].pipelineLayoutIndex;
            if (plIndex < static_cast<uint32_t>(pl_.rhr.size())) {
                PLUGIN_ASSERT(plIndex < static_cast<uint32_t>(pl_.rhr.size()));
                return pl_.rhr[plIndex];
            }
        }
    }
    return {};
}

RenderHandleReference ShaderManager::GetPipelineLayoutHandleByShaderHandle(const RenderHandleReference& handle) const
{
    return GetPipelineLayoutHandleByShaderHandle(handle.GetHandle());
}

RenderHandleReference ShaderManager::GetPipelineLayoutHandle(const string_view name) const
{
    if (const auto iter = pl_.nameToIndex.find(name); iter != pl_.nameToIndex.cend()) {
        const uint32_t index = iter->second;
        PLUGIN_ASSERT(index < static_cast<uint32_t>(pl_.rhr.size()));
        return pl_.rhr[index];
    } else {
        PLUGIN_LOG_W("ShaderManager: named pipeline layout not found: %s", name.data());
        return {};
    }
}

PipelineLayout ShaderManager::GetPipelineLayout(const RenderHandle& handle) const
{
    return GetPipelineLayoutRef(handle);
}

PipelineLayout ShaderManager::GetPipelineLayout(const RenderHandleReference& handle) const
{
    return GetPipelineLayoutRef(handle.GetHandle());
}

const PipelineLayout& ShaderManager::GetPipelineLayoutRef(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if ((type == RenderHandleType::PIPELINE_LAYOUT) && (index < static_cast<uint32_t>(pl_.data.size()))) {
        return pl_.data[index];
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (RenderHandleUtil::IsValid(handle) && (type != RenderHandleType::PIPELINE_LAYOUT)) {
            PLUGIN_LOG_W("RENDER_VALIDATION: invalid handle type given to GetPipelineLayout()");
        }
#endif
        return defaultPipelineLayout_;
    }
}

RenderHandleReference ShaderManager::GetReflectionPipelineLayoutHandle(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    uint32_t plIndex = INVALID_SM_INDEX;
    if (type == RenderHandleType::SHADER_STATE_OBJECT) {
        if (arrayIndex < shaderMappings_.clientData.size()) {
            plIndex = shaderMappings_.clientData[arrayIndex].reflectionPipelineLayoutIndex;
        }
    } else if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        if (arrayIndex < computeShaderMappings_.clientData.size()) {
            plIndex = computeShaderMappings_.clientData[arrayIndex].reflectionPipelineLayoutIndex;
        }
    }

    if (plIndex < pl_.data.size()) {
        return pl_.rhr[plIndex];
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: ShaderManager, invalid shader handle for GetReflectionPipelineLayoutHandle");
#endif
        return {};
    }
}

RenderHandleReference ShaderManager::GetReflectionPipelineLayoutHandle(const RenderHandleReference& handle) const
{
    return GetReflectionPipelineLayoutHandle(handle.GetHandle());
}

PipelineLayout ShaderManager::GetReflectionPipelineLayout(const RenderHandleReference& handle) const
{
    return GetReflectionPipelineLayoutRef(handle.GetHandle());
}

const PipelineLayout& ShaderManager::GetReflectionPipelineLayoutRef(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    uint32_t plIndex = INVALID_SM_INDEX;
    if (type == RenderHandleType::SHADER_STATE_OBJECT) {
        if (arrayIndex < shaderMappings_.clientData.size()) {
            plIndex = shaderMappings_.clientData[arrayIndex].reflectionPipelineLayoutIndex;
        }
    } else if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        if (arrayIndex < computeShaderMappings_.clientData.size()) {
            plIndex = computeShaderMappings_.clientData[arrayIndex].reflectionPipelineLayoutIndex;
        }
    }

    if (plIndex < pl_.data.size()) {
        return pl_.data[plIndex];
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: ShaderManager, invalid shader handle for GetReflectionPipelineLayout");
#endif
        return defaultPipelineLayout_;
    }
}

ShaderSpecilizationConstantView ShaderManager::GetReflectionSpecialization(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (type == RenderHandleType::SHADER_STATE_OBJECT) {
        // NOTE: at the moment there might not be availability yet, will be FIXED
        if (arrayIndex < shaders_.size()) {
            if (shaders_[arrayIndex].gsp) {
                return shaders_[arrayIndex].gsp->GetReflection().shaderSpecializationConstantView;
            }
        }
    } else if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        // NOTE: at the moment there might not be availability yet, will be FIXED
        if (arrayIndex < computeShaders_.size()) {
            if (computeShaders_[arrayIndex].gsp) {
                return computeShaders_[arrayIndex].gsp->GetReflection().shaderSpecializationConstantView;
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_W("RENDER_VALIDATION: ShaderManager, invalid shader handle for GetReflectionSpecialization");
#endif
    return defaultSSCV_;
}

ShaderSpecilizationConstantView ShaderManager::GetReflectionSpecialization(const RenderHandleReference& handle) const
{
    return GetReflectionSpecialization(handle.GetHandle());
}

VertexInputDeclarationView ShaderManager::GetReflectionVertexInputDeclaration(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (type == RenderHandleType::SHADER_STATE_OBJECT) {
        // NOTE: at the moment there might not be availability yet, will be FIXED
        if (arrayIndex < shaders_.size()) {
            if (shaders_[arrayIndex].gsp) {
                return shaders_[arrayIndex].gsp->GetReflection().vertexInputDeclarationView;
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_W("RENDER_VALIDATION: ShaderManager, invalid shader handle for GetReflectionVertexInputDeclaration");
#endif
    return defaultVIDV_;
}

VertexInputDeclarationView ShaderManager::GetReflectionVertexInputDeclaration(const RenderHandleReference& handle) const
{
    return GetReflectionVertexInputDeclaration(handle.GetHandle());
}

ShaderThreadGroup ShaderManager::GetReflectionThreadGroupSize(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        // NOTE: at the moment there might not be availability yet, will be FIXED
        if (arrayIndex < computeShaders_.size()) {
            if (computeShaders_[arrayIndex].gsp) {
                const auto& refl = computeShaders_[arrayIndex].gsp->GetReflection();
                return { refl.threadGroupSizeX, refl.threadGroupSizeY, refl.threadGroupSizeZ };
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_W("RENDER_VALIDATION: ShaderManager, invalid shader handle for GetReflectionThreadGroupSize");
#endif
    return defaultSTG_;
}

ShaderThreadGroup ShaderManager::GetReflectionThreadGroupSize(const RenderHandleReference& handle) const
{
    return GetReflectionThreadGroupSize(handle.GetHandle());
}

RenderHandleReference ShaderManager::CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo)
{
    uint32_t arrayIndex = INVALID_SM_INDEX;
    if (auto nameIter = pl_.nameToIndex.find(createInfo.path); nameIter != pl_.nameToIndex.end()) {
        PLUGIN_ASSERT(nameIter->second < pl_.rhr.size());
        arrayIndex = static_cast<uint32_t>(nameIter->second);
    }

    if (arrayIndex < static_cast<uint32_t>(pl_.data.size())) { // replace
        // inside core validation due to being very low info for common users
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_I("ShaderManager: re-creating pipeline layout (name %s)", createInfo.path.data());
#endif
    } else { // new
        arrayIndex = static_cast<uint32_t>(pl_.data.size());
        pl_.data.emplace_back(PipelineLayout {});
        // NOTE: only updated for new (should check with re-creation)
        if (!createInfo.path.empty()) {
            pl_.nameToIndex[createInfo.path] = arrayIndex;
        }
        pl_.rhr.emplace_back(RenderHandleReference {});
    }

    if (arrayIndex < static_cast<uint32_t>(pl_.data.size())) {
        const PipelineLayout& pipelineLayout = createInfo.pipelineLayout;
        PipelineLayout& ref = pl_.data[arrayIndex];
#if (RENDER_VALIDATION_ENABLED == 1)
        if (pipelineLayout.descriptorSetCount > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT &&
            pipelineLayout.pushConstant.byteSize > PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE) {
            PLUGIN_LOG_W(
                "Invalid pipeline layout sizes clamped (name:%s). Set count %u <= %u, push constant size %u <= %u",
                createInfo.path.data(), ref.descriptorSetCount, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT,
                pipelineLayout.pushConstant.byteSize, PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE);
        }
#endif
        ref.pushConstant = pipelineLayout.pushConstant;
        ref.descriptorSetCount =
            Math::min(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT, pipelineLayout.descriptorSetCount);
        ref.pushConstant.byteSize =
            Math::min(PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE, pipelineLayout.pushConstant.byteSize);
        uint32_t descriptorSetBitmask = 0;
        // can be user generated pipeline layout (i.e. set index might be different than index)
        for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
            const uint32_t setIdx = pipelineLayout.descriptorSetLayouts[idx].set;
            if (setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
                ref.descriptorSetLayouts[setIdx] = pipelineLayout.descriptorSetLayouts[setIdx];
                descriptorSetBitmask |= (1 << setIdx);
            }
        }

        const RenderHandle handle =
            RenderHandleUtil::CreateHandle(RenderHandleType::PIPELINE_LAYOUT, arrayIndex, 0, descriptorSetBitmask);
        pl_.rhr[arrayIndex] = RenderHandleReference(handle, IRenderReferenceCounter::Ptr(new ShaderReferenceCounter()));
        return pl_.rhr[arrayIndex];
    } else {
        return {};
    }
}

const GpuComputeProgram* ShaderManager::GetGpuComputeProgram(const RenderHandle& handle) const
{
    if (!IsComputeShaderFunc(handle)) {
        PLUGIN_LOG_E("ShaderManager: invalid compute shader handle");
        return nullptr;
    }
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < static_cast<uint32_t>(computeShaders_.size())) {
        return computeShaders_[index].gsp.get();
    } else {
        PLUGIN_LOG_E("ShaderManager: invalid compute shader handle");
        return nullptr;
    }
}

const GpuShaderProgram* ShaderManager::GetGpuShaderProgram(const RenderHandle& handle) const
{
    if (!IsShaderFunc(handle)) {
        PLUGIN_LOG_E("ShaderManager: invalid shader handle");
        return nullptr;
    }
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < static_cast<uint32_t>(shaders_.size())) {
        return shaders_[index].gsp.get();
    } else {
        PLUGIN_LOG_E("ShaderManager: invalid shader handle");
        return nullptr;
    }
}

uint32_t ShaderManager::CreateShaderModule(const string_view name, const ShaderModuleCreateInfo& createInfo)
{
    auto& nameToIdx = shaderModules_.nameToIndex;
    auto& modules = shaderModules_.shaderModules;
    if (auto iter = nameToIdx.find(name); iter != nameToIdx.end()) {
        PLUGIN_ASSERT(iter->second < modules.size());
        // inside core validation due to being very low info for common users
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_I("ShaderManager: re-creating shader module of name %s", name.data());
#endif
        // check that we don't push the same indices multiple times
        bool found = false;
        for (const auto& ref : pendingAllocations_.recreatedShaderModuleIndices) {
            if (ref == iter->second) {
                found = true;
                break;
            }
        }
        if (!found) {
            pendingAllocations_.recreatedShaderModuleIndices.emplace_back(iter->second);
        }
        deferredDestructions_.shaderModules.push_back({ device_.GetFrameCount(), move(modules[iter->second]) });
        modules[iter->second] = device_.CreateShaderModule(createInfo);
        return iter->second;
    } else {
        const uint32_t idx = static_cast<uint32_t>(modules.size());
        if (!name.empty()) {
            nameToIdx[name] = idx;
        }
        modules.emplace_back(device_.CreateShaderModule(createInfo));
        return idx;
    }
}

ShaderModule* ShaderManager::GetShaderModule(const uint32_t index) const
{
    const auto& modules = shaderModules_.shaderModules;
    if (index < modules.size()) {
        return modules[index].get();
    } else {
        return nullptr;
    }
}

uint32_t ShaderManager::GetShaderModuleIndex(const string_view name) const
{
    const auto& nameToIdx = shaderModules_.nameToIndex;
    if (const auto iter = nameToIdx.find(name); iter != nameToIdx.cend()) {
        PLUGIN_ASSERT(iter->second < shaderModules_.shaderModules.size());
        return iter->second;
    } else {
        return INVALID_SM_INDEX;
    }
}

bool ShaderManager::IsComputeShader(const RenderHandleReference& handle) const
{
    return IsComputeShaderFunc(handle.GetHandle());
}

bool ShaderManager::IsShader(const RenderHandleReference& handle) const
{
    return IsShaderFunc(handle.GetHandle());
}

void ShaderManager::LoadShaderFiles(const ShaderFilePathDesc& desc)
{
    if (shaderLoader_) {
        shaderLoader_->Load(desc);
    }
}

void ShaderManager::LoadShaderFile(const string_view uri)
{
    if (shaderLoader_ && (!uri.empty())) {
        shaderLoader_->LoadFile(uri, false);
    }
}

void ShaderManager::UnloadShaderFiles(const ShaderFilePathDesc& desc) {}

void ShaderManager::ReloadShaderFile(const string_view uri)
{
    if (shaderLoader_ && (!uri.empty())) {
        shaderLoader_->LoadFile(uri, true);
        hasReloadedShaders_ = true;
    }
}

void ShaderManager::ReloadSpvFiles(const array_view<string>& spvFiles)
{
    if (shaderLoader_ && (!spvFiles.empty())) {
        shaderLoader_->Reload(spvFiles);
        hasReloadedShaders_ = true;
    }
}

bool ShaderManager::HasReloadedShaders() const
{
    return hasReloadedShaders_;
}

const json::value* ShaderManager::GetMaterialMetadata(const RenderHandleReference& handle) const
{
    if (const auto iter = shaderToMetadata_.find(handle.GetHandle()); iter != shaderToMetadata_.end()) {
        return &iter->second.json;
    }
    return nullptr;
}

void ShaderManager::DestroyShader(const RenderHandle handle)
{
    auto eraseIndexData = [](auto& mapStore, const RenderHandle handle) {
        if (auto const pos = std::find_if(
                mapStore.begin(), mapStore.end(), [handle](auto const& element) { return element.second == handle; });
            pos != mapStore.end()) {
            mapStore.erase(pos);
        }
    };

    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (IsComputeShaderFunc(handle)) {
        auto& mappings = computeShaderMappings_;
        if (index < static_cast<uint32_t>(mappings.clientData.size())) {
            mappings.clientData[index] = {};
            eraseIndexData(nameToClientHandle_, handle);
            {
                const auto lock = std::lock_guard(pendingMutex_);
                pendingAllocations_.destroyHandles.emplace_back(handle);
            }
        }
    } else if (IsShaderFunc(handle)) {
        auto& mappings = shaderMappings_;
        if (index < static_cast<uint32_t>(mappings.clientData.size())) {
            mappings.clientData[index] = {};
            eraseIndexData(nameToClientHandle_, handle);
            {
                const auto lock = std::lock_guard(pendingMutex_);
                pendingAllocations_.destroyHandles.emplace_back(handle);
            }
        }
    }
}

void ShaderManager::Destroy(const RenderHandleReference& handle)
{
    const RenderHandle rawHandle = handle.GetHandle();
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(rawHandle);
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) ||
        (handleType == RenderHandleType::SHADER_STATE_OBJECT)) {
        DestroyShader(rawHandle);
    } else if (handleType == RenderHandleType::GRAPHICS_STATE) {
        DestroyGraphicsState(rawHandle);
    } else if (handleType == RenderHandleType::PIPELINE_LAYOUT) {
        DestroyPipelineLayout(rawHandle);
    } else if (handleType == RenderHandleType::VERTEX_INPUT_DECLARATION) {
        DestroyVertexInputDeclaration(rawHandle);
    }
}

void ShaderManager::DestroyGraphicsState(const RenderHandle handle)
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < static_cast<uint32_t>(graphicsStates_.rhr.size())) {
        graphicsStates_.rhr[index] = {};
        graphicsStates_.data[index] = {};
        graphicsStates_.graphicsStates[index] = {};

        auto eraseIndexData = [](auto& mapStore, const uint32_t index) {
            if (auto const pos = std::find_if(
                    mapStore.begin(), mapStore.end(), [index](auto const& element) { return element.second == index; });
                pos != mapStore.end()) {
                mapStore.erase(pos);
            }
        };
        eraseIndexData(graphicsStates_.nameToIndex, index);
        eraseIndexData(graphicsStates_.hashToIndex, index);
        // NOTE: shaderToStates needs to be added
    }
}

void ShaderManager::DestroyPipelineLayout(const RenderHandle handle)
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < static_cast<uint32_t>(pl_.rhr.size())) {
        pl_.rhr[index] = {};
        pl_.data[index] = {};

        auto eraseIndexData = [](auto& mapStore, const uint32_t index) {
            if (auto const pos = std::find_if(
                    mapStore.begin(), mapStore.end(), [index](auto const& element) { return element.second == index; });
                pos != mapStore.end()) {
                mapStore.erase(pos);
            }
        };
        eraseIndexData(pl_.nameToIndex, index);
        eraseIndexData(pl_.computeShaderToIndex, index);
        eraseIndexData(pl_.shaderToIndex, index);
    }
}

void ShaderManager::DestroyVertexInputDeclaration(const RenderHandle handle)
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < static_cast<uint32_t>(shaderVid_.rhr.size())) {
        shaderVid_.rhr[index] = {};
        shaderVid_.data[index] = {};

        auto eraseIndexData = [](auto& mapStore, const uint32_t index) {
            if (auto const pos = std::find_if(
                    mapStore.begin(), mapStore.end(), [index](auto const& element) { return element.second == index; });
                pos != mapStore.end()) {
                mapStore.erase(pos);
            }
        };
        eraseIndexData(shaderVid_.nameToIndex, index);
        eraseIndexData(shaderVid_.shaderToIndex, index);
    }
}

vector<RenderHandleReference> ShaderManager::GetShaders(
    const RenderHandleReference& handle, const ShaderStageFlags shaderStageFlags) const
{
    vector<RenderHandleReference> shaders;
    if ((shaderStageFlags &
            (CORE_SHADER_STAGE_VERTEX_BIT | CORE_SHADER_STAGE_FRAGMENT_BIT | CORE_SHADER_STAGE_COMPUTE_BIT)) == 0) {
        return shaders;
    }
    const RenderHandleType handleType = handle.GetHandleType();
    const uint32_t handleIndex = RenderHandleUtil::GetIndexPart(handle.GetHandle());
    if (handleType == RenderHandleType::GRAPHICS_STATE) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: GetShaders with graphics state handle not supported");
#endif
    } else if ((handleType == RenderHandleType::PIPELINE_LAYOUT) ||
               (handleType == RenderHandleType::VERTEX_INPUT_DECLARATION)) {
        if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT) {
            for (const auto& ref : computeShaderMappings_.clientData) {
                if (ref.pipelineLayoutIndex == handleIndex) {
                    shaders.emplace_back(ref.rhr);
                }
            }
        }
        if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS) {
            for (const auto& ref : shaderMappings_.clientData) {
                if (ref.vertexInputDeclarationIndex == handleIndex) {
                    shaders.emplace_back(ref.rhr);
                }
            }
        }
    }
    return shaders;
}

vector<RenderHandle> ShaderManager::GetShaders(
    const RenderHandle& handle, const ShaderStageFlags shaderStageFlags) const
{
    vector<RenderHandle> shaders;
    if ((shaderStageFlags &
            (CORE_SHADER_STAGE_VERTEX_BIT | CORE_SHADER_STAGE_FRAGMENT_BIT | CORE_SHADER_STAGE_COMPUTE_BIT)) == 0) {
        return shaders;
    }
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    const uint32_t handleIndex = RenderHandleUtil::GetIndexPart(handle);
    if (handleType == RenderHandleType::GRAPHICS_STATE) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: GetShaders with graphics state handle not supported");
#endif
    } else if ((handleType == RenderHandleType::PIPELINE_LAYOUT) ||
               (handleType == RenderHandleType::VERTEX_INPUT_DECLARATION)) {
        if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT) {
            for (const auto& ref : computeShaderMappings_.clientData) {
                if (ref.pipelineLayoutIndex == handleIndex) {
                    shaders.emplace_back(ref.rhr.GetHandle());
                }
            }
        }
        if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS) {
            for (const auto& ref : shaderMappings_.clientData) {
                if (ref.vertexInputDeclarationIndex == handleIndex) {
                    shaders.emplace_back(ref.rhr.GetHandle());
                }
            }
        }
    }
    return shaders;
}

IShaderManager::IdDesc ShaderManager::GetShaderIdDesc(const RenderHandle handle) const
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    IdDesc desc;
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) &&
        (index < static_cast<uint32_t>(computeShaderMappings_.clientData.size()))) {
        desc.renderSlotId = computeShaderMappings_.clientData[index].renderSlotId;
        for (const auto& ref : nameToClientHandle_) {
            if (ref.second == handle) {
                desc.path = ref.first;
            }
        }
    } else if ((handleType == RenderHandleType::SHADER_STATE_OBJECT) &&
               (index < static_cast<uint32_t>(shaderMappings_.clientData.size()))) {
        desc.renderSlotId = shaderMappings_.clientData[index].renderSlotId;
        for (const auto& ref : nameToClientHandle_) {
            if (ref.second == handle) {
                desc.path = ref.first;
            }
        }
    }
    return desc;
}

IShaderManager::IdDesc ShaderManager::GetIdDesc(const RenderHandleReference& handle) const
{
    auto GetIdDesc = [](const auto& nameToIndex, const auto handleIndex) {
        IdDesc desc;
        for (const auto& ref : nameToIndex) {
            if (ref.second == handleIndex) {
                desc.path = ref.first;
            }
        }
        return desc;
    };
    const RenderHandle rawHandle = handle.GetHandle();
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(rawHandle);
    const uint32_t handleIndex = RenderHandleUtil::GetIndexPart(rawHandle);
    IdDesc desc;
    if ((handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) ||
        (handleType == RenderHandleType::SHADER_STATE_OBJECT)) {
        desc = GetShaderIdDesc(rawHandle);
    } else if ((handleType == RenderHandleType::GRAPHICS_STATE) && (handleIndex < graphicsStates_.rhr.size())) {
        desc = GetIdDesc(graphicsStates_.nameToIndex, handleIndex);
    } else if ((handleType == RenderHandleType::PIPELINE_LAYOUT) && (handleIndex < pl_.rhr.size())) {
        desc = GetIdDesc(pl_.nameToIndex, handleIndex);
    } else if ((handleType == RenderHandleType::VERTEX_INPUT_DECLARATION) && (handleIndex < shaderVid_.rhr.size())) {
        desc = GetIdDesc(shaderVid_.nameToIndex, handleIndex);
    }
    return desc;
}

IShaderPipelineBinder::Ptr ShaderManager::CreateShaderPipelineBinder(const RenderHandleReference& handle) const
{
    const RenderHandleType type = handle.GetHandleType();
    if (handle &&
        ((type == RenderHandleType::SHADER_STATE_OBJECT) || (type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT))) {
        return IShaderPipelineBinder::Ptr { new ShaderPipelineBinder(handle, GetReflectionPipelineLayout(handle)) };
    }
    return nullptr;
}

ShaderManager::CompatibilityFlags ShaderManager::GetCompatibilityFlags(
    const RenderHandle& lhs, const RenderHandle& rhs) const
{
    const RenderHandleType lType = RenderHandleUtil::GetHandleType(lhs);
    const RenderHandleType rType = RenderHandleUtil::GetHandleType(rhs);
    CompatibilityFlags flags = 0;
    // NOTE: only same types supported at the moment
    if (lType == rType) {
        if (lType == RenderHandleType::PIPELINE_LAYOUT) {
            const PipelineLayout lpl = GetPipelineLayout(lhs);
            const PipelineLayout rpl = GetPipelineLayout(rhs);
            flags = GetPipelineLayoutCompatibilityFlags(lpl, rpl);
        } else if ((lType == RenderHandleType::SHADER_STATE_OBJECT) ||
                   (lType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT)) {
            // first check that given pipeline layout is valid to own reflection
            const RenderHandle shaderPlHandle = GetPipelineLayoutHandleByShaderHandle(rhs).GetHandle();
            if (RenderHandleUtil::IsValid(shaderPlHandle)) {
                const PipelineLayout shaderPl = GetPipelineLayout(shaderPlHandle);
                const PipelineLayout rpl = GetReflectionPipelineLayoutRef(rhs);
                if (rpl.descriptorSetCount > 0) {
                    flags = GetPipelineLayoutCompatibilityFlags(rpl, shaderPl);
                }
            }
            // then, compare to lhs with rhs reflection
            if (flags != 0) {
                const RenderHandle lShaderPlHandle = GetPipelineLayoutHandleByShaderHandle(lhs).GetHandle();
                const PipelineLayout lpl = RenderHandleUtil::IsValid(lShaderPlHandle)
                                               ? GetPipelineLayout(lShaderPlHandle)
                                               : GetReflectionPipelineLayoutRef(lhs);
                flags = GetPipelineLayoutCompatibilityFlags(lpl, GetReflectionPipelineLayoutRef(rhs));
            }
        }
    }
    return flags;
}

ShaderManager::CompatibilityFlags ShaderManager::GetCompatibilityFlags(
    const RenderHandleReference& lhs, const RenderHandleReference& rhs) const
{
    if (lhs && rhs) {
        return GetCompatibilityFlags(lhs.GetHandle(), rhs.GetHandle());
    } else {
        return CompatibilityFlags { 0 };
    }
}

void ShaderManager::SetFileManager(IFileManager& fileMgr)
{
    fileMgr_ = &fileMgr;
    shaderLoader_ = make_unique<ShaderLoader>(*fileMgr_, *this, device_.GetBackendType());
}

constexpr uint8_t REFLECTION_TAG[] = { 'r', 'f', 'l', 0 };
struct ReflectionHeader {
    uint8_t tag[sizeof(REFLECTION_TAG)];
    uint16_t type;
    uint16_t offsetPushConstants;
    uint16_t offsetSpecializationConstants;
    uint16_t offsetDescriptorSets;
    uint16_t offsetInputs;
    uint16_t offsetLocalSize;
};

bool ShaderReflectionData::IsValid() const
{
    if (reflectionData.size() < sizeof(ReflectionHeader)) {
        return false;
    }
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    return memcmp(header.tag, REFLECTION_TAG, sizeof(REFLECTION_TAG)) == 0;
}

ShaderStageFlags ShaderReflectionData::GetStageFlags() const
{
    ShaderStageFlags flags;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    flags = header.type;
    return flags;
}

PipelineLayout ShaderReflectionData::GetPipelineLayout() const
{
    PipelineLayout pipelineLayout;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetPushConstants && header.offsetPushConstants < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetPushConstants;
        const auto constants = *ptr;
        if (constants) {
            pipelineLayout.pushConstant.shaderStageFlags = header.type;
            pipelineLayout.pushConstant.byteSize = static_cast<uint32_t>(*(ptr + 1) | (*(ptr + 2) << 8));
        }
    }
    if (header.offsetDescriptorSets && header.offsetDescriptorSets < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetDescriptorSets;
        pipelineLayout.descriptorSetCount = static_cast<uint32_t>(*(ptr) | (*(ptr + 1) << 8));
        ptr += 2;
        for (auto i = 0u; i < pipelineLayout.descriptorSetCount; ++i) {
            // write to correct set location
            const uint32_t set = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
            PLUGIN_ASSERT(set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
            auto& layout = pipelineLayout.descriptorSetLayouts[set];
            layout.set = set;
            ptr += 2;
            const auto bindings = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
            ptr += 2;
            for (auto j = 0u; j < bindings; ++j) {
                DescriptorSetLayoutBinding binding;
                binding.binding = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
                ptr += 2;
                binding.descriptorType = static_cast<DescriptorType>(*ptr | (*(ptr + 1) << 8));
                if ((binding.descriptorType > DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) &&
                    (binding.descriptorType ==
                        (DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE & 0xffff))) {
                    binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
                }
                ptr += 2;
                binding.descriptorCount = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8));
                ptr += 2;
                binding.shaderStageFlags = header.type;
                layout.bindings.push_back(binding);
            }
        }
    }
    return pipelineLayout;
}

vector<ShaderSpecialization::Constant> ShaderReflectionData::GetSpecializationConstants() const
{
    vector<ShaderSpecialization::Constant> constants;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetSpecializationConstants && header.offsetSpecializationConstants < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetSpecializationConstants;
        const auto size = *ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24;
        ptr += 4;
        for (auto i = 0; i < size; ++i) {
            ShaderSpecialization::Constant constant;
            constant.shaderStage = header.type;
            constant.id = static_cast<uint32_t>(*ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24);
            ptr += 4;
            constant.type = static_cast<ShaderSpecialization::Constant::Type>(
                *ptr | *(ptr + 1) << 8 | *(ptr + 2) << 16 | *(ptr + 3) << 24);
            ptr += 4;
            constant.offset = 0;
            constants.push_back(constant);
        }
    }
    return constants;
}

vector<VertexInputDeclaration::VertexInputAttributeDescription> ShaderReflectionData::GetInputDescriptions() const
{
    vector<VertexInputDeclaration::VertexInputAttributeDescription> inputs;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetInputs && header.offsetInputs < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetInputs;
        const auto size = *(ptr) | (*(ptr + 1) << 8);
        ptr += 2;
        for (auto i = 0; i < size; ++i) {
            VertexInputDeclaration::VertexInputAttributeDescription desc;
            desc.location = static_cast<uint32_t>(*(ptr) | (*(ptr + 1) << 8));
            ptr += 2;
            desc.binding = desc.location;
            desc.format = static_cast<Format>(*(ptr) | (*(ptr + 1) << 8));
            ptr += 2;
            desc.offset = 0;
            inputs.push_back(desc);
        }
    }
    return inputs;
}

Math::UVec3 ShaderReflectionData::GetLocalSize() const
{
    Math::UVec3 sizes;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetLocalSize && header.offsetLocalSize < reflectionData.size()) {
        auto ptr = reflectionData.data() + header.offsetLocalSize;
        sizes.x = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
        ptr += 4;
        sizes.y = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
        ptr += 4;
        sizes.z = static_cast<uint32_t>(*ptr | (*(ptr + 1) << 8) | (*(ptr + 2)) << 16 | (*(ptr + 3)) << 24);
    }
    return sizes;
}

const uint8_t* ShaderReflectionData::GetPushConstants() const
{
    const uint8_t* ptr = nullptr;
    const ReflectionHeader& header = *reinterpret_cast<const ReflectionHeader*>(reflectionData.data());
    if (header.offsetPushConstants && header.offsetPushConstants < reflectionData.size()) {
        const auto constants = *(reflectionData.data() + header.offsetPushConstants);
        if (constants) {
            // number of constants is uint8 and the size of the constant is uint16
            ptr = reflectionData.data() + header.offsetPushConstants + sizeof(uint8_t) + sizeof(uint16_t);
        }
    }
    return ptr;
}

RenderNodeShaderManager::RenderNodeShaderManager(const ShaderManager& shaderMgr) : shaderMgr_(shaderMgr) {}

RenderHandle RenderNodeShaderManager::GetShaderHandle(const string_view name) const
{
    return shaderMgr_.GetShaderHandle(name).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetShaderHandle(const string_view name, const string_view variantName) const
{
    return shaderMgr_.GetShaderHandle(name, variantName).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetShaderHandle(const RenderHandle& handle, const uint32_t renderSlotId) const
{
    return shaderMgr_.GetShaderHandle(handle, renderSlotId).GetHandle();
}

vector<RenderHandle> RenderNodeShaderManager::GetShaders(const uint32_t renderSlotId) const
{
    return shaderMgr_.GetShaderRawHandles(renderSlotId);
}

RenderHandle RenderNodeShaderManager::GetGraphicsStateHandle(const string_view name) const
{
    return shaderMgr_.GetGraphicsStateHandle(name).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetGraphicsStateHandle(
    const string_view name, const string_view variantName) const
{
    return shaderMgr_.GetGraphicsStateHandle(name, variantName).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetGraphicsStateHandle(
    const RenderHandle& handle, const uint32_t renderSlotId) const
{
    return shaderMgr_.GetGraphicsStateHandle(handle, renderSlotId).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetGraphicsStateHandleByHash(const uint64_t hash) const
{
    return shaderMgr_.GetGraphicsStateHandleByHash(hash).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetGraphicsStateHandleByShaderHandle(const RenderHandle& handle) const
{
    return shaderMgr_.GetGraphicsStateHandleByShaderHandle(handle).GetHandle();
}

const GraphicsState& RenderNodeShaderManager::GetGraphicsState(const RenderHandle& handle) const
{
    return shaderMgr_.GetGraphicsStateRef(handle);
}

uint32_t RenderNodeShaderManager::GetRenderSlotId(const string_view renderSlot) const
{
    return shaderMgr_.GetRenderSlotId(renderSlot);
}

uint32_t RenderNodeShaderManager::GetRenderSlotId(const RenderHandle& handle) const
{
    return shaderMgr_.GetRenderSlotId(handle);
}

IShaderManager::RenderSlotData RenderNodeShaderManager::GetRenderSlotData(const uint32_t renderSlotId) const
{
    return shaderMgr_.GetRenderSlotData(renderSlotId);
}

RenderHandle RenderNodeShaderManager::GetVertexInputDeclarationHandleByShaderHandle(const RenderHandle& handle) const
{
    return shaderMgr_.GetVertexInputDeclarationHandleByShaderHandle(handle).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetVertexInputDeclarationHandle(const string_view name) const
{
    return shaderMgr_.GetVertexInputDeclarationHandle(name).GetHandle();
}

VertexInputDeclarationView RenderNodeShaderManager::GetVertexInputDeclarationView(const RenderHandle& handle) const
{
    return shaderMgr_.GetVertexInputDeclarationView(handle);
}

RenderHandle RenderNodeShaderManager::GetPipelineLayoutHandleByShaderHandle(const RenderHandle& handle) const
{
    return shaderMgr_.GetPipelineLayoutHandleByShaderHandle(handle).GetHandle();
}

const PipelineLayout& RenderNodeShaderManager::GetPipelineLayout(const RenderHandle& handle) const
{
    return shaderMgr_.GetPipelineLayoutRef(handle);
}

RenderHandle RenderNodeShaderManager::GetPipelineLayoutHandle(const string_view name) const
{
    return shaderMgr_.GetPipelineLayoutHandle(name).GetHandle();
}

RenderHandle RenderNodeShaderManager::GetReflectionPipelineLayoutHandle(const RenderHandle& handle) const
{
    return shaderMgr_.GetReflectionPipelineLayoutHandle(handle).GetHandle();
}

const PipelineLayout& RenderNodeShaderManager::GetReflectionPipelineLayout(const RenderHandle& handle) const
{
    return shaderMgr_.GetReflectionPipelineLayoutRef(handle);
}

ShaderSpecilizationConstantView RenderNodeShaderManager::GetReflectionSpecialization(const RenderHandle& handle) const
{
    return shaderMgr_.GetReflectionSpecialization(handle);
}

VertexInputDeclarationView RenderNodeShaderManager::GetReflectionVertexInputDeclaration(
    const RenderHandle& handle) const
{
    return shaderMgr_.GetReflectionVertexInputDeclaration(handle);
}

ShaderThreadGroup RenderNodeShaderManager::GetReflectionThreadGroupSize(const RenderHandle& handle) const
{
    return shaderMgr_.GetReflectionThreadGroupSize(handle);
}

uint64_t RenderNodeShaderManager::HashGraphicsState(const GraphicsState& graphicsState) const
{
    return shaderMgr_.HashGraphicsState(graphicsState);
}

bool RenderNodeShaderManager::IsValid(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsValid(handle);
}

bool RenderNodeShaderManager::IsComputeShader(const RenderHandle& handle) const
{
    return IsComputeShaderFunc(handle);
}

bool RenderNodeShaderManager::IsShader(const RenderHandle& handle) const
{
    return IsShaderFunc(handle);
}

vector<RenderHandle> RenderNodeShaderManager::GetShaders(
    const RenderHandle& handle, const ShaderStageFlags shaderStageFlags) const
{
    return shaderMgr_.GetShaders(handle, shaderStageFlags);
}

IShaderManager::CompatibilityFlags RenderNodeShaderManager::GetCompatibilityFlags(
    const RenderHandle& lhs, const RenderHandle& rhs) const
{
    return shaderMgr_.GetCompatibilityFlags(lhs, rhs);
}
RENDER_END_NAMESPACE()
