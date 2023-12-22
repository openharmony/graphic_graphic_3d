/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "mesh_builder.h"

#include <algorithm>
#include <securec.h>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/util/intf_picking.h>
#include <base/math/float_packer.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_handle.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

namespace {
#include "3d/shaders/common/morph_target_structs.h"
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr uint32_t BUFFER_ALIGN = 0x100; // on Nvidia = 0x20, on Mali and Intel = 0x10, SBO on Mali = 0x100

template<typename Container>
using ContainerValueType = typename remove_reference_t<Container>::value_type;

template<typename Container>
constexpr const auto COUNT_OF_VALUE_TYPE_V = std::extent_v<decltype(ContainerValueType<Container>::data)>;

template<typename Container>
constexpr const auto SIZE_OF_VALUE_TYPE_V = sizeof(ContainerValueType<Container>);

template<typename Container>
constexpr auto ValueTypeCount(Container container)
{
    return COUNT_OF_VALUE_TYPE_V<Container>;
}

size_t Align(size_t value, size_t align)
{
    if (value == 0) {
        return 0;
    }

    return ((value + align) / align) * align;
}

const VertexInputDeclaration::VertexInputAttributeDescription* GetVertexAttributeDescription(uint32_t location,
    const array_view<const VertexInputDeclaration::VertexInputAttributeDescription>& attributeDescriptions)
{
    const auto cmpLocationIndex = [location](auto const& attribute) { return attribute.location == location; };
    if (const auto pos = std::find_if(attributeDescriptions.begin(), attributeDescriptions.end(), cmpLocationIndex);
        pos != attributeDescriptions.end()) {
        return pos.ptr();
    }

    return nullptr;
}

const VertexInputDeclaration::VertexInputBindingDescription* GetVertexBindingeDescription(uint32_t bindingIndex,
    const array_view<const VertexInputDeclaration::VertexInputBindingDescription>& bindingDescriptions)
{
    const auto cmpBindingIndex = [bindingIndex](auto const& binding) { return binding.binding == bindingIndex; };
    if (const auto pos = std::find_if(bindingDescriptions.begin(), bindingDescriptions.end(), cmpBindingIndex);
        pos != bindingDescriptions.end()) {
        return pos.ptr();
    }

    return nullptr;
}

struct FormatProperties {
    Format format;
    size_t componentCount;
    size_t componentByteSize;
    bool isNormalized;
    bool isSigned;
};

static constexpr const FormatProperties DATA_FORMATS[] = {
    { BASE_FORMAT_R8G8B8A8_UNORM, 4, 1, true, false },
    { BASE_FORMAT_R8G8B8A8_UINT, 4, 1, false, false },
    { BASE_FORMAT_R16_SFLOAT, 1, 2, false, true },
    { BASE_FORMAT_R16G16_SFLOAT, 2, 2, false, true },
    { BASE_FORMAT_R16G16B16_SNORM, 2, 2, true, true },
    { BASE_FORMAT_R16G16B16_SFLOAT, 3, 2, false, true },
    { BASE_FORMAT_R16G16B16A16_SNORM, 4, 2, true, true },
    { BASE_FORMAT_R16G16B16A16_UINT, 4, 2, false, false },
    { BASE_FORMAT_R16G16B16A16_SFLOAT, 4, 2, false, true },
    { BASE_FORMAT_R32_SFLOAT, 1, 4, false, true },
    { BASE_FORMAT_R32G32_SFLOAT, 2, 4, false, true },
    { BASE_FORMAT_R32G32B32_SFLOAT, 3, 4, false, true },
    { BASE_FORMAT_R32G32B32A32_UINT, 4, 4, false, false },
    { BASE_FORMAT_R32G32B32A32_SFLOAT, 4, 4, false, true },
};

bool GetFormatSpec(
    Format format, size_t& outComponentCount, size_t& outComponentByteSize, bool& outNormalized, bool& outSigned)
{
#if defined(__cpp_lib_constexpr_algorithms) && (__cpp_lib_constexpr_algorithms >= 201806L)
    static_assert(std::is_sorted(std::begin(DATA_FORMATS), std::end(DATA_FORMATS),
        [](const FormatProperties& lhs, const FormatProperties& rhs) { return lhs.format < rhs.format; }));
#endif
    if (auto pos = std::lower_bound(std::begin(DATA_FORMATS), std::end(DATA_FORMATS), format,
            [](const FormatProperties& element, Format value) { return element.format < value; });
        (pos != std::end(DATA_FORMATS)) && (pos->format == format)) {
        outComponentCount = pos->componentCount;
        outComponentByteSize = pos->componentByteSize;
        outNormalized = pos->isNormalized;
        outSigned = pos->isSigned;
        return true;
    }
    outNormalized = false;
    outSigned = true;
    outComponentCount = 0;
    outComponentByteSize = 0;
    return false;
}

void ConvertFloatDataTo8bitSnorm(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    const float* src = data;

    int8_t* dst8 = reinterpret_cast<int8_t*>(out.data());

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            // 32-bit floating point value to normalized signed 8-bit integer value.
            float v = src[cmp];
            const float round = v >= 0.f ? +.5f : -.5f;
            v = v < -1.f ? -1.f : (v > 1.f ? 1.f : v);
            dst8[cmp] = static_cast<int8_t>(v * 127.f + round);
        }
        src += componentCount;
        dst8 += targetComponentCount;
    }
}

void ConvertFloatDataTo8bitUnorm(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    const float* src = data;

    uint8_t* dst8 = reinterpret_cast<uint8_t*>(out.data());

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            // 32-bit floating point value to normalized unsigned 8-bit integer value.
            const float v = src[cmp];
            dst8[cmp] = static_cast<uint8_t>(v * 255.f + 0.5f);
        }
        src += componentCount;
        dst8 += targetComponentCount;
    }
}

template<bool normalized, bool isSigned>
void ConvertFloatDataTo8bit(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    if (normalized) {
        if (isSigned) {
            ConvertFloatDataTo8bitSnorm(data, componentCount, elementCount, targetComponentCount, out);
        } else {
            ConvertFloatDataTo8bitUnorm(data, componentCount, elementCount, targetComponentCount, out);
        }
    } else {
        CORE_ASSERT(normalized);
    }
}

void ConvertFloatDataTo16bitSnorm(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    const float* src = data;

    int16_t* dst16 = reinterpret_cast<int16_t*>(out.data());

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            // 32-bit floating point value to normalized signed 16-bit integer value.
            float v = src[cmp];
            const float round = v >= 0.f ? +.5f : -.5f;
            v = v < -1.f ? -1.f : (v > 1.f ? 1.f : v);

            dst16[cmp] = static_cast<int16_t>(v * 32767.f + round);
        }
        src += componentCount;
        dst16 += targetComponentCount;
    }
}

void ConvertFloatDataTo16bitUnorm(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    const float* src = data;

    uint16_t* dst16 = reinterpret_cast<uint16_t*>(out.data());

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            // 32-bit floating point value to normalized unsigned 16-bit integer value.
            dst16[cmp] = static_cast<uint16_t>(src[cmp] * 65535.0f + 0.5f);
        }
        src += componentCount;
        dst16 += targetComponentCount;
    }
}

void ConvertFloatDataTo16bitFloat(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    const float* src = data;

    uint16_t* dst16 = reinterpret_cast<uint16_t*>(out.data());

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            // 32-bit floating point value to 16-bit floating point value.
            dst16[cmp] = Math::F32ToF16(src[cmp]);
        }
        src += componentCount;
        dst16 += targetComponentCount;
    }
}

template<bool normalized, bool isSigned>
void ConvertFloatDataTo16bit(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, vector<uint8_t>& out)
{
    if (normalized) {
        if (isSigned) {
            ConvertFloatDataTo16bitSnorm(data, componentCount, elementCount, targetComponentCount, out);
        } else {
            ConvertFloatDataTo16bitUnorm(data, componentCount, elementCount, targetComponentCount, out);
        }
    } else {
        ConvertFloatDataTo16bitFloat(data, componentCount, elementCount, targetComponentCount, out);
    }
}

void ConvertFloatDataTo(const float* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, const size_t targetComponentSize, const bool normalized, const bool isSigned,
    vector<uint8_t>& out)
{
    CORE_ASSERT_MSG(componentCount <= targetComponentCount, "Invalid component count (%zu vs. %zu).", componentCount,
        targetComponentCount);

    // Allocate memory.
    out.resize(targetComponentCount * targetComponentSize * elementCount);
    switch (targetComponentSize) {
        case 1: // 1: target component size == 1
            if (normalized) {
                if (isSigned) {
                    ConvertFloatDataTo8bit<true, true>(data, componentCount, elementCount, targetComponentCount, out);
                } else {
                    ConvertFloatDataTo8bit<true, false>(data, componentCount, elementCount, targetComponentCount, out);
                }
            } else {
                CORE_ASSERT(normalized);
            }
            break;
        case 2: // 2: target component size == 2
            if (normalized) {
                if (isSigned) {
                    ConvertFloatDataTo16bit<true, true>(data, componentCount, elementCount, targetComponentCount, out);
                } else {
                    ConvertFloatDataTo16bit<true, false>(data, componentCount, elementCount, targetComponentCount, out);
                }
            } else {
                ConvertFloatDataTo16bit<false, false>(data, componentCount, elementCount, targetComponentCount, out);
            }
            break;
        default:
            break;
    }
}

void ConvertByteDataTo(const uint8_t* data, const size_t componentCount, const size_t elementCount,
    const size_t targetComponentCount, const size_t targetComponentSize, const bool normalized, const bool isSigned,
    vector<uint8_t>& out)
{
    CORE_ASSERT_MSG(!normalized, "Format not supported.");
    CORE_ASSERT_MSG(componentCount <= targetComponentCount, "Invalid component count (%zu vs. %zu).", componentCount,
        targetComponentCount);

    // Allocate memory.
    size_t targetElementSize = targetComponentCount * targetComponentSize;
    out.resize(targetElementSize * elementCount);

    const uint8_t* src = data;
    uint8_t* dst = out.data();

    for (size_t i = 0; i < elementCount; ++i) {
        for (size_t cmp = 0; cmp < componentCount; ++cmp) {
            if (targetComponentSize == sizeof(uint16_t)) {
                // Convert to unsigned short.
                uint16_t* p = reinterpret_cast<uint16_t*>(&(dst[cmp]));
                *p = src[cmp];
            } else if (targetComponentSize == sizeof(uint32_t)) {
                // Convert to unsigned int.
                uint32_t* p = reinterpret_cast<uint32_t*>(&(dst[cmp]));
                *p = src[cmp];
            }
        }

        src += componentCount;
        dst += targetElementSize;
    }
}

size_t GetVertexAttributeByteSize(
    const uint32_t vertexAttributeLocation, const VertexInputDeclarationView& vertexInputDeclaration)
{
    if (const auto* vertexAttributeDesc =
            GetVertexAttributeDescription(vertexAttributeLocation, vertexInputDeclaration.attributeDescriptions);
        vertexAttributeDesc) {
        size_t componentCount = 0;
        size_t componentByteSize = 0;
        bool normalized = false;
        bool isSigned = false;
        if (!GetFormatSpec(vertexAttributeDesc->format, componentCount, componentByteSize, normalized, isSigned)) {
            CORE_ASSERT_MSG(false, "Format not supported (%u).", vertexAttributeDesc->format);
        }
        return componentCount * componentByteSize;
    }
    return 0;
}

void CopyAttributeData(const void* dst, const void* src, const size_t offset, const size_t stride,
    const size_t elementSize, const size_t elementCount)
{
    uintptr_t source = (uintptr_t)src;
    uintptr_t destination = (uintptr_t)dst;

    destination += offset;

    if (elementSize == stride) {
        if (!CloneData(reinterpret_cast<void*>(destination), elementCount * elementSize,
                reinterpret_cast<void*>(source), elementCount * elementSize)) {
            CORE_LOG_E("attributeData copying failed.");
        }
    } else {
        for (size_t i = 0; i < elementCount; i++) {
            if (!CloneData(reinterpret_cast<void*>(destination), (elementCount - i) * stride,
                    reinterpret_cast<void*>(source), elementSize)) {
                CORE_LOG_E("attributeData element copying failed.");
            }
            destination += stride;
            source += elementSize;
        }
    }
}

void CopyMorphNormalAttributeData(const void* dst, const void* src, const size_t offset, const size_t stride,
    const size_t elementSize, const size_t elementCount, const size_t srcSize, const size_t srcOffset)
{
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
    vector<uint8_t> out;
    ConvertFloatDataTo(reinterpret_cast<const float*>(src) + srcOffset, 3u, srcSize, 3u, 2u, false, true, out);
    CopyAttributeData(dst, out.data(), offset + offsetof(MorphInputData, nortan), stride, elementSize, elementCount);
#else
    CopyAttributeData(dst, reinterpret_cast<const float*>(src) + srcOffset, offset + offsetof(MorphInputData, nor),
        stride, elementSize, elementCount);
#endif
}

void CopyMorphTangentAttributeData(const void* dst, const void* src, const size_t offset, const size_t stride,
    const size_t elementSize, const size_t elementCount, const size_t srcSize, const size_t srcOffset)
{
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
    vector<uint8_t> out;
    constexpr auto componentSize = 2u;
    // base tangents have 4 components but morph target tangent deltas have only 3. dividing the elementSize by
    // componentSize gives the componentCount.
    ConvertFloatDataTo(reinterpret_cast<const float*>(src) + srcOffset, elementSize / componentSize, srcSize,
        elementSize / componentSize, componentSize, false, true, out);
    CopyAttributeData(
        dst, out.data(), offset + offsetof(MorphInputData, nortan) + 8u, stride, elementSize, elementCount);
#else
    CopyAttributeData(dst, reinterpret_cast<const float*>(src) + srcOffset, offset + offsetof(MorphInputData, tan),
        stride, elementSize, elementCount);
#endif
}

// For each joint 6 values defining the min and max bounds (world space AABB) of the vertices affected by the joint.
constexpr const size_t JOINT_BOUNDS_COMPONENTS = 6u;

GpuBufferDesc GetVertexBufferDesc(size_t byteSize, bool morphTarget)
{
    // NOTE: storage buffer usage is currently enabled for all
    // there's no API to define flags for auto loaded and build meshes
    // one might always want more (and e.g. with particle cases we should use texel storage for auto formats)
    GpuBufferDesc desc;
    desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_ENABLE_MEMORY_OPTIMIZATIONS;
    if (morphTarget) {
        desc.engineCreationFlags |= CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    }
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.byteSize = (uint32_t)byteSize;

    return desc;
}

template<typename T>
constexpr GpuBufferDesc GetIndexBufferDesc(size_t indexCount)
{
    // NOTE: storage buffer usage is currently enabled for all
    // there's no API to define flags for auto loaded and build meshes
    // one might always want more (and e.g. with particle cases we should use texel storage for auto formats)
    GpuBufferDesc indexBufferDesc;
    indexBufferDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                 BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    indexBufferDesc.engineCreationFlags = 0;
    indexBufferDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    indexBufferDesc.byteSize = sizeof(T) * (uint32_t)indexCount;

    return indexBufferDesc;
}

constexpr GpuBufferDesc GetMorphTargetBufferDesc(size_t byteSize)
{
    // NOTE: These are only morph targets which are read only
    GpuBufferDesc desc;
    desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    desc.engineCreationFlags = 0; // read-only
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.byteSize = (uint32_t)byteSize;

    return desc;
}

EntityReference CreateBuffer(IEntityManager& entityManager, IRenderHandleComponentManager& handleManager,
    IGpuResourceManager& gpuResourceManager, const GpuBufferDesc& vertexBufferDesc,
    array_view<const uint8_t> vertexData_)
{
    auto sharedEntity = entityManager.CreateReferenceCounted();
    handleManager.Create(sharedEntity);
    handleManager.Write(sharedEntity)->reference = gpuResourceManager.Create(vertexBufferDesc, vertexData_);
    return sharedEntity;
}

void FillSubmeshBuffers(array_view<MeshComponent::Submesh> submeshes, const MeshBuilder::BufferEntities& bufferEntities)
{
    for (MeshComponent::Submesh& submesh : submeshes) {
        submesh.indexBuffer.buffer = bufferEntities.indexBuffer;
        submesh.morphTargetBuffer.buffer = bufferEntities.morphBuffer;

        submesh.bufferAccess[MeshComponent::Submesh::DM_VB_POS].buffer = bufferEntities.vertexBuffer;
        submesh.bufferAccess[MeshComponent::Submesh::DM_VB_NOR].buffer = bufferEntities.vertexBuffer;
        submesh.bufferAccess[MeshComponent::Submesh::DM_VB_UV0].buffer = bufferEntities.vertexBuffer;

        if (submesh.flags & MeshComponent::Submesh::FlagBits::SECOND_TEXCOORD_BIT) {
            submesh.bufferAccess[MeshComponent::Submesh::DM_VB_UV1].buffer = bufferEntities.vertexBuffer;
        }

        if (submesh.flags & MeshComponent::Submesh::FlagBits::TANGENTS_BIT) {
            submesh.bufferAccess[MeshComponent::Submesh::DM_VB_TAN].buffer = bufferEntities.vertexBuffer;
        }
        if (submesh.flags & MeshComponent::Submesh::FlagBits::VERTEX_COLORS_BIT) {
            submesh.bufferAccess[MeshComponent::Submesh::DM_VB_COL].buffer = bufferEntities.vertexBuffer;
        }

        if (submesh.flags & MeshComponent::Submesh::FlagBits::SKIN_BIT) {
            submesh.bufferAccess[MeshComponent::Submesh::DM_VB_JOI].buffer = bufferEntities.jointBuffer;
            submesh.bufferAccess[MeshComponent::Submesh::DM_VB_JOW].buffer = bufferEntities.jointBuffer;
        }
    }
}

MinAndMax CalculateAabb(array_view<const MeshComponent::Submesh> submeshes)
{
    MinAndMax minMax;
    for (const auto& submesh : submeshes) {
        minMax.minAABB = min(minMax.minAABB, submesh.aabbMin);
        minMax.maxAABB = max(minMax.maxAABB, submesh.aabbMax);
    }
    return minMax;
}
} // namespace

void MeshBuilder::Initialize(const VertexInputDeclarationView& vertexInputDeclaration, size_t submeshCount)
{
    submeshInfos_.clear();
    submeshes_.clear();

    vertexCount_ = 0;
    indexCount_ = 0;

    vertexData_.clear();
    jointData_.clear();
    indexData_.clear();
    targetData_.clear();
    jointBoundsData_.clear();

    vertexInputDeclaration_ = vertexInputDeclaration;
    submeshInfos_.reserve(submeshCount);
    submeshes_.resize(submeshCount);
}

uint32_t MeshBuilder::GetVertexCount() const
{
    return vertexCount_;
}

uint32_t MeshBuilder::GetIndexCount() const
{
    return indexCount_;
}

const MeshBuilder::Submesh& MeshBuilder::GetSubmesh(size_t index) const
{
    return submeshInfos_[index].info;
}

void MeshBuilder::AddSubmesh(const Submesh& info)
{
    SubmeshExt submeshInfo;
    submeshInfo.info = info;

    submeshInfos_.emplace_back(submeshInfo);
}

MeshBuilder::BufferSizesInBytes MeshBuilder::CalculateSizes()
{
    BufferSizesInBytes bufferSizes {};

    const size_t jointIndexSizeInBytes =
        GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOI, vertexInputDeclaration_);
    const size_t jointWeightSizeInBytes =
        GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOW, vertexInputDeclaration_);

    for (auto& submesh : submeshInfos_) {
        submesh.vertexIndex = (uint32_t)vertexCount_;

        // Calculate vertex binding sizes.
        submesh.vertexBindingByteSize.resize(vertexInputDeclaration_.bindingDescriptions.size());
        submesh.vertexBindingOffset.resize(vertexInputDeclaration_.bindingDescriptions.size());
        for (auto const& bindingDesc : vertexInputDeclaration_.bindingDescriptions) {
            submesh.vertexBindingByteSize[bindingDesc.binding] =
                static_cast<uint32_t>(Align(bindingDesc.stride * submesh.info.vertexCount, BUFFER_ALIGN));
        }

        submesh.indexBufferOffset = (uint32_t)Align(bufferSizes.indexBuffer, BUFFER_ALIGN);
        submesh.jointBufferOffset = (uint32_t)Align(bufferSizes.jointBuffer, BUFFER_ALIGN);
        submesh.morphTargetBufferOffset = (uint32_t)Align(bufferSizes.morphVertexData, BUFFER_ALIGN);

        if (submesh.info.indexType == CORE_INDEX_TYPE_UINT16) {
            bufferSizes.indexBuffer = submesh.indexBufferOffset + (submesh.info.indexCount * sizeof(uint16_t));
        } else {
            bufferSizes.indexBuffer = submesh.indexBufferOffset + (submesh.info.indexCount * sizeof(uint32_t));
        }

        if (submesh.info.joints) {
            const size_t currJointIndexByteSize = Align(jointIndexSizeInBytes * submesh.info.vertexCount, BUFFER_ALIGN);
            const size_t currJointWeightByteSize =
                Align(jointWeightSizeInBytes * submesh.info.vertexCount, BUFFER_ALIGN);
            // joint index and joint weight bytesizes both need to be aligned
            bufferSizes.jointBuffer = submesh.jointBufferOffset + currJointIndexByteSize + currJointWeightByteSize;
        }

        if (submesh.info.morphTargetCount > 0) {
            submesh.morphTargets.resize(submesh.info.morphTargetCount);
            // vertexCount * uint32_t * morphTargetCount, index/indexOffset to sparse target data
            // vertexCount * MorphInputData, base data
            // vertexCount * MorphInputData * morphTargetCount, target data
            const uint32_t indexSize = (uint32_t)Align(
                submesh.info.vertexCount * submesh.info.morphTargetCount * sizeof(uint32_t), BUFFER_ALIGN);
            const uint32_t targetSize = (uint32_t)Align(
                submesh.info.vertexCount * sizeof(MorphInputData) * (submesh.info.morphTargetCount + 1u), BUFFER_ALIGN);
            bufferSizes.morphVertexData = submesh.morphTargetBufferOffset + indexSize + targetSize;
        }

        vertexCount_ += submesh.info.vertexCount;
    }
    return bufferSizes;
}

void MeshBuilder::Allocate()
{
    BufferSizesInBytes bufferSizes = CalculateSizes();
    bufferSizes.indexBuffer = Align(bufferSizes.indexBuffer, BUFFER_ALIGN);
    bufferSizes.jointBuffer = Align(bufferSizes.jointBuffer, BUFFER_ALIGN);
    bufferSizes.morphVertexData = Align(bufferSizes.morphVertexData, BUFFER_ALIGN);

    indexCount_ = (uint32_t)bufferSizes.indexBuffer / sizeof(uint32_t);

    uint32_t vertexBufferSizeInBytes = 0;

    // Set binding offsets.
    for (auto const& bindingDesc : vertexInputDeclaration_.bindingDescriptions) {
        for (auto& submesh : submeshInfos_) {
            submesh.vertexBindingOffset[bindingDesc.binding] = vertexBufferSizeInBytes;
            vertexBufferSizeInBytes += submesh.vertexBindingByteSize[bindingDesc.binding];
        }
    }

    // Allocate memory.
    vertexData_.resize(vertexBufferSizeInBytes);
    indexData_.resize(bufferSizes.indexBuffer);
    jointData_.resize(bufferSizes.jointBuffer);
    targetData_.resize(bufferSizes.morphVertexData);
}

void MeshBuilder::WriteFloatAttributeData(const void* destination, const void* source, size_t bufferOffset,
    size_t stride, size_t componentCount, size_t componentByteSize, size_t elementCount, Format targetFormat)
{
    const void* data = source;
    size_t elementByteSize = componentCount * componentByteSize;

    // Convert to proper format, if needed.
    size_t targetComponentCount = 0;
    size_t targetComponentSize = 0;
    bool normalized = false;
    bool isSigned = false;
    if (!GetFormatSpec(targetFormat, targetComponentCount, targetComponentSize, normalized, isSigned)) {
        CORE_ASSERT_MSG(false, "Format not supported (%u).", targetFormat);
    }

    vector<uint8_t> convertedData;
    if ((componentByteSize != targetComponentSize) || (componentCount != targetComponentCount)) {
        // Convert data to requested format.
        ConvertFloatDataTo(reinterpret_cast<const float*>(data), componentCount, elementCount, targetComponentCount,
            targetComponentSize, normalized, isSigned, convertedData);

        // Grab data and new element size.
        data = convertedData.data();
        elementByteSize = targetComponentCount * targetComponentSize;
    }

    CopyAttributeData(destination, data, bufferOffset, stride, elementByteSize, elementCount);
}

void MeshBuilder::WriteByteAttributeData(const void* destination, const void* source, size_t bufferOffset,
    size_t stride, size_t componentCount, size_t componentByteSize, size_t elementCount, Format targetFormat)
{
    const void* data = source;
    size_t elementByteSize = componentCount * componentByteSize;

    // Convert to proper format, if needed.
    size_t targetComponentCount = 0;
    size_t targetComponentSize = 0;
    bool normalized = false;
    bool isSigned = false;
    if (!GetFormatSpec(targetFormat, targetComponentCount, targetComponentSize, normalized, isSigned)) {
        CORE_ASSERT_MSG(false, "Format not supported (%u).", targetFormat);
    }

    vector<uint8_t> convertedData;
    if ((componentByteSize != targetComponentSize) || (componentCount != targetComponentCount)) {
        // Convert data to requested format.
        ConvertByteDataTo(reinterpret_cast<const uint8_t*>(data), componentCount, elementCount, targetComponentCount,
            targetComponentSize, normalized, isSigned, convertedData);

        // Grab data and new element size.
        data = convertedData.data();
        elementByteSize = targetComponentCount * targetComponentSize;
    }

    CopyAttributeData(destination, data, bufferOffset, stride, elementByteSize, elementCount);
}

void MeshBuilder::SetVertexData(size_t submeshIndex, const array_view<const Math::Vec3>& positions,
    const array_view<const Math::Vec3>& normals, const array_view<const Math::Vec2>& texcoords0,
    const array_view<const Math::Vec2>& texcoords1, const array_view<const Math::Vec4>& tangents,
    const array_view<const Math::Vec4>& colors)
{
    SetVertexData(submeshIndex,
        array_view<const float>(
            reinterpret_cast<const float*>(positions.data()), positions.size() * ValueTypeCount(positions)),
        array_view<const float>(
            reinterpret_cast<const float*>(normals.data()), normals.size() * ValueTypeCount(normals)),
        array_view<const float>(
            reinterpret_cast<const float*>(texcoords0.data()), texcoords0.size() * ValueTypeCount(texcoords0)),
        array_view<const float>(
            reinterpret_cast<const float*>(texcoords1.data()), texcoords1.size() * ValueTypeCount(texcoords1)),
        array_view<const float>(
            reinterpret_cast<const float*>(tangents.data()), tangents.size() * ValueTypeCount(tangents)),
        array_view<const float>(reinterpret_cast<const float*>(colors.data()), colors.size() * ValueTypeCount(colors)));
}

void MeshBuilder::SetVertexData(size_t submeshIndex, const array_view<const float>& positions,
    const array_view<const float>& normals, const array_view<const float>& texcoords0,
    const array_view<const float>& texcoords1, const array_view<const float>& tangents,
    const array_view<const float>& colors)
{
    const SubmeshExt& submesh = submeshInfos_[submeshIndex];

    // Submesh info for this submesh.
    MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];

    submeshDesc.material = submesh.info.material;
    submeshDesc.vertexCount = submesh.info.vertexCount;
    submeshDesc.instanceCount = submesh.info.instanceCount;

    // Process position.
    {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_POS];
        WriteData(positions, 3u, submesh, MeshComponent::Submesh::DM_VB_POS, acc.offset, acc.byteSize);
    }

    // Process normal.
    {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_NOR];
        WriteData(normals, 3u, submesh, MeshComponent::Submesh::DM_VB_NOR, acc.offset, acc.byteSize);
    }

    // Process uv.
    {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV0];
        WriteData(texcoords0, 2u, submesh, MeshComponent::Submesh::DM_VB_UV0, acc.offset, acc.byteSize);
    }
    if (!texcoords1.empty()) {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV1];
        if (WriteData(texcoords1, 2u, submesh, MeshComponent::Submesh::DM_VB_UV1, acc.offset, acc.byteSize)) {
            submeshDesc.flags |= MeshComponent::Submesh::FlagBits::SECOND_TEXCOORD_BIT;
        }
    } else {
        const auto& uv0 = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV0];
        auto& uv1 = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV1];
        uv1 = uv0;
    }

    // Process tangent.
    if (!tangents.empty() && submesh.info.tangents) {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_TAN];
        if (WriteData(tangents, 4u, submesh, MeshComponent::Submesh::DM_VB_TAN, acc.offset, acc.byteSize)) {
            submeshDesc.flags |= MeshComponent::Submesh::FlagBits::TANGENTS_BIT;
        }
    }

    // Process vertex colors.
    if (!colors.empty() && submesh.info.colors) {
        auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_COL];
        if (WriteData(colors, 4u, submesh, MeshComponent::Submesh::DM_VB_COL, acc.offset, acc.byteSize)) {
            submeshDesc.flags |= MeshComponent::Submesh::FlagBits::VERTEX_COLORS_BIT;
        }
    }
}

bool MeshBuilder::WriteData(const array_view<const float>& data, uint32_t componentCount, const SubmeshExt& submesh,
    uint32_t attributeLocation, uint32_t& byteOffset, uint32_t& byteSize)
{
    if (const VertexInputDeclaration::VertexInputAttributeDescription* attributeDesc =
            GetVertexAttributeDescription(attributeLocation, vertexInputDeclaration_.attributeDescriptions);
        attributeDesc) {
        if (const VertexInputDeclaration::VertexInputBindingDescription* bindingDesc =
                GetVertexBindingeDescription(attributeDesc->binding, vertexInputDeclaration_.bindingDescriptions);
            bindingDesc) {
            // this offset and size should be aligned
            byteOffset = submesh.vertexBindingOffset[bindingDesc->binding] + attributeDesc->offset;
            byteSize = submesh.info.vertexCount * bindingDesc->stride;
            WriteFloatAttributeData(vertexData_.data(), data.data(), byteOffset, bindingDesc->stride, componentCount,
                sizeof(float), data.size() / componentCount, attributeDesc->format);
            return true;
        }
    }
    return false;
}

void MeshBuilder::SetAABB(size_t submeshIndex, const Math::Vec3& min, const Math::Vec3& max)
{
    MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
    submeshDesc.aabbMin = min;
    submeshDesc.aabbMax = max;
}

void MeshBuilder::CalculateAABB(size_t submeshIndex, const array_view<const Math::Vec3>& positions)
{
    constexpr float maxLimits = std::numeric_limits<float>::max();
    constexpr float minLimits = -std::numeric_limits<float>::max();

    Math::Vec3 minimum = { maxLimits, maxLimits, maxLimits };
    Math::Vec3 maximum = { minLimits, minLimits, minLimits };

    for (size_t i = 0; i < positions.size(); ++i) {
        const Math::Vec3& data = positions[i];

        minimum = min(minimum, data);
        maximum = max(maximum, data);
    }

    SetAABB(submeshIndex, minimum, maximum);
}

void MeshBuilder::SetIndexData(size_t submeshIndex, const array_view<const uint8_t>& indices)
{
    MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
    const SubmeshExt& submesh = submeshInfos_[submeshIndex];

    uint32_t const indexSize =
        (submesh.info.indexType == IndexType::CORE_INDEX_TYPE_UINT16) ? sizeof(uint16_t) : sizeof(uint32_t);

    size_t const loadedIndexCount = indices.size() / indexSize;
    if (loadedIndexCount) {
        uint8_t* dst = &(indexData_[submesh.indexBufferOffset]);
        if (!CloneData(dst, indexData_.size() - submesh.indexBufferOffset, indices.data(), indices.size())) {
            CORE_LOG_E("copying of indexdata failed");
        }
    }

    submeshDesc.indexCount = (uint32_t)loadedIndexCount;
    submeshDesc.indexBuffer.indexType = submesh.info.indexType;
    submeshDesc.indexBuffer.offset = submesh.indexBufferOffset;
    submeshDesc.indexBuffer.byteSize = (uint32_t)indices.size();
}

void MeshBuilder::SetJointData(size_t submeshIndex, const array_view<const uint8_t>& jointData,
    const array_view<const Math::Vec4>& weightData, const array_view<const Math::Vec3>& vertexPositions)
{
    if (!jointData.empty() && !weightData.empty()) {
        if (const auto* indexAttributeDesc = GetVertexAttributeDescription(
                MeshComponent::Submesh::DM_VB_JOI, vertexInputDeclaration_.attributeDescriptions);
            indexAttributeDesc) {
            if (const auto* weightAttributeDesc = GetVertexAttributeDescription(
                    MeshComponent::Submesh::DM_VB_JOW, vertexInputDeclaration_.attributeDescriptions);
                weightAttributeDesc) {
                const size_t jointIndexByteSize =
                    GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOI, vertexInputDeclaration_);
                const size_t jointWeightByteSize =
                    GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOW, vertexInputDeclaration_);

                // Process joint indices, assume data in vec(byte, byte, byte, byte) format.
                MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
                const SubmeshExt& submesh = submeshInfos_[submeshIndex];
                auto& jointIndexAcc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_JOI];
                jointIndexAcc.offset = (uint32_t)Align(submesh.jointBufferOffset, BUFFER_ALIGN);
                jointIndexAcc.byteSize = (uint32_t)jointIndexByteSize * submesh.info.vertexCount;
                WriteByteAttributeData(jointData_.data(), jointData.data(), jointIndexAcc.offset, jointIndexByteSize,
                    4, 1, submesh.info.vertexCount, indexAttributeDesc->format); // 4: count of components

                // Process joint weights.
                auto& jointWeightAcc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_JOW];
                // index aligned offset + index bytesize -> aligned to offset
                jointWeightAcc.offset = (uint32_t)Align(jointIndexAcc.offset + jointIndexAcc.byteSize, BUFFER_ALIGN);
                jointWeightAcc.byteSize = (uint32_t)jointWeightByteSize * submesh.info.vertexCount;

                if (weightAttributeDesc->format == BASE_FORMAT_R8G8B8A8_UNORM) {
                    // Make sure the sum of the weights is still 1.0 after conversion to unorm bytes.
                    vector<uint8_t> weightDataUnorm;
                    const size_t weightCount = weightData.size();
                    weightDataUnorm.resize(weightCount * 4u);
                    for (size_t i = 0; i < weightCount; i++) {
                        const auto weights = (weightData[i] * 255.0f) + Math::Vec4(0.5f, 0.5f, 0.5f, 0.5f);
                        uint8_t v[] = { (uint8_t)weights.x, (uint8_t)weights.y, (uint8_t)weights.z,
                            (uint8_t)weights.w };
                        constexpr int EXPECTED_SUM = 255;
                        const int sum = v[0] + v[1] + v[2] + v[3];
                        if (sum != EXPECTED_SUM) {
                            // Compensate the error in the sum on the largest weight.
                            const int diff = EXPECTED_SUM - sum;
                            auto max = std::max_element(std::begin(v), std::end(v));
                            *max = (uint8_t)(*max + diff);
                        }
                        CloneData(weightDataUnorm.data() + i * 4u, weightDataUnorm.size() - i * 4u, v, 4u);
                    }
                    WriteByteAttributeData(jointData_.data(), weightDataUnorm.data(), jointWeightAcc.offset,
                        jointWeightByteSize, 4, // 4: count of components
                        1, submesh.info.vertexCount, weightAttributeDesc->format);
                } else {
                    WriteFloatAttributeData(jointData_.data(), weightData.data(), jointWeightAcc.offset,
                        jointWeightByteSize, 4, sizeof(float), // 4: count of components
                        submesh.info.vertexCount, weightAttributeDesc->format);
                }

                submeshDesc.flags |= MeshComponent::Submesh::FlagBits::SKIN_BIT;
            }
        }

        CalculateJointBounds(weightData, jointData, vertexPositions);
    }
}

void MeshBuilder::CalculateJointBounds(const array_view<const Math::Vec4>& weightData,
    const array_view<const uint8_t>& jointData, const array_view<const Math::Vec3>& vertexPositions)
{
    // Calculate joint bounds as the bounds of the vertices that the joint references.
    const float* weights = &weightData[0].x;

    const size_t jointIndexCount = jointData.size();
    CORE_ASSERT(jointData.size() == weightData.size() * 4u);
    CORE_ASSERT(jointData.size() == vertexPositions.size() * 4u);

    // Find the amount of referenced joints
    size_t maxJointIndex = 0;
    for (size_t i = 0; i < jointIndexCount; i++) {
        if (weights[i] < Math::EPSILON) {
            // Ignore joints with weight that is effectively 0.0
            continue;
        }

        const uint8_t jointIndex = jointData[i];
        if (jointIndex > maxJointIndex) {
            maxJointIndex = jointIndex;
        }
    }

    // Make sure bounds data is big enough. Initialize new bounds to min and max values.
    const size_t oldSize = jointBoundsData_.size();
    const size_t newSize = (maxJointIndex + 1);
    if (newSize > 0 && newSize > oldSize) {
        constexpr float floatMin = std::numeric_limits<float>::lowest();
        constexpr float floatMax = std::numeric_limits<float>::max();

        constexpr const Bounds minMax = { { floatMax, floatMax, floatMax }, { floatMin, floatMin, floatMin } };
        jointBoundsData_.resize(newSize, minMax);
    }

    for (size_t i = 0; i < jointIndexCount; i++) {
        if (weights[i] < Math::EPSILON) {
            // Ignore joints with weight that is effectively 0.0
            continue;
        }

        // Each vertex can reference 4 joint indices.
        const size_t vertexIndex = i / 4u;

        auto& boundsData = jointBoundsData_[jointData[i]];
        boundsData.min = Math::min(boundsData.min, vertexPositions[vertexIndex]);
        boundsData.max = Math::max(boundsData.max, vertexPositions[vertexIndex]);
    }
}

void MeshBuilder::GatherDeltas(SubmeshExt& submesh, uint32_t baseOffset, uint32_t indexOffset, uint32_t targetSize,
    const array_view<const Math::Vec3> targetPositions, const array_view<const Math::Vec3> targetNormals,
    const array_view<const Math::Vec3> targetTangents)
{
    // Target data starts after base
    uint32_t targetOffset = baseOffset + targetSize;

    auto index = reinterpret_cast<uint32_t*>(targetData_.data() + indexOffset);
    for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
        submesh.morphTargets[trg].offset = targetOffset;
        const auto startTarget = reinterpret_cast<MorphInputData*>(targetData_.data() + targetOffset);
        auto target = startTarget;
        for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
            // for each vertex in target check that position, normal and tangent deltas are non-zero.
            const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
            if (targetPositions[vertexIndex] != Math::Vec3 {} ||
                (!targetNormals.empty() && targetNormals[vertexIndex] != Math::Vec3 {}) ||
                (!targetTangents.empty() && targetTangents[vertexIndex] != Math::Vec3 {})) {
                // store offset for each non-zero
                *index++ = (targetOffset - baseOffset) / sizeof(MorphInputData);
                targetOffset += sizeof(MorphInputData);
                target->pos = Math::Vec4(targetPositions[vertexIndex], static_cast<float>(vertex));

                if (!targetNormals.empty()) {
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                    target->nortan.x =
                        Math::PackHalf2X16({ targetNormals[vertexIndex].x, targetNormals[vertexIndex].y });
                    target->nortan.y = Math::PackHalf2X16({ targetNormals[vertexIndex].z, 0.f });
#else
                    target->nor = Math::Vec4(targetNormals[vertexIndex], 0.f);
#endif
                }
                if (!targetTangents.empty()) {
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                    target->nortan.z =
                        Math::PackHalf2X16({ targetTangents[vertexIndex].x, targetTangents[vertexIndex].y });
                    target->nortan.w = Math::PackHalf2X16({ targetTangents[vertexIndex].z, 0.f });
#else
                    target->tan = Math::Vec4(targetTangents[vertexIndex], 0.f);
#endif
                }
                ++target;
            } else {
                // invalid offset for zero deltas
                *index++ = UINT32_MAX;
            }
        }
        // Store the size and indexOffset of the gathered deltas.
        const auto byteSize =
            static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
        submesh.morphTargets[trg].byteSize = byteSize;
    }
}

void MeshBuilder::SetMorphTargetData(size_t submeshIndex, const array_view<const Math::Vec3>& basePositions,
    const array_view<const Math::Vec3>& baseNormals, const array_view<const Math::Vec4>& baseTangents,
    const array_view<const Math::Vec3>& targetPositions, const array_view<const Math::Vec3>& targetNormals,
    const array_view<const Math::Vec3>& targetTangents)
{
    // NOTE: Use vertex input info once morph system supports != FLOAT.
    constexpr uint32_t positionByteSize = SIZE_OF_VALUE_TYPE_V<decltype(basePositions)>;
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
    constexpr uint32_t normalByteSize = SIZE_OF_VALUE_TYPE_V<decltype(baseNormals)> / 2u;
    constexpr uint32_t tangentByteSize = SIZE_OF_VALUE_TYPE_V<decltype(baseTangents)> / 2u;
#else
    constexpr uint32_t normalByteSize = SIZE_OF_VALUE_TYPE_V<decltype(baseNormals)>;
    constexpr uint32_t tangentByteSize = SIZE_OF_VALUE_TYPE_V<decltype(baseTangents)>;
#endif

    // Submesh info for this submesh.
    SubmeshExt& submesh = submeshInfos_[submeshIndex];
    if (submesh.info.morphTargetCount > 0) {
        // Offset to index data is previous offset + size (or zero for the first submesh)
        uint32_t indexOffset = 0u;
        if (submeshIndex) {
            indexOffset = (uint32_t)Align(submeshInfos_[submeshIndex - 1u].morphTargetBufferOffset +
                                              submeshInfos_[submeshIndex - 1u].morphTargetBufferSize,
                BUFFER_ALIGN);
        }
        submesh.morphTargetBufferOffset = indexOffset;

        // 32bit index/offset for each vertex in each morph target
        const uint32_t indexSize = sizeof(uint32_t) * submesh.info.vertexCount;
        const uint32_t totalIndexSize = (uint32_t)Align(indexSize * submesh.info.morphTargetCount, BUFFER_ALIGN);

        // Data struct (pos, nor, tan) for each vertex. total amount is target size for each target data and one base
        // data
        const uint32_t targetSize = submesh.info.vertexCount * sizeof(MorphInputData);
        const uint32_t totalTargetSize =
            (uint32_t)Align(targetSize * (submesh.info.morphTargetCount + 1u), BUFFER_ALIGN);

        // Make sure buffer fits non-sparse data
        const uint32_t maxSize = totalIndexSize + totalTargetSize;
        targetData_.resize(indexOffset + maxSize);

        // Base data starts after index data
        const uint32_t baseOffset = indexOffset + totalIndexSize;

        CopyAttributeData(targetData_.data(), basePositions.data(), baseOffset + offsetof(MorphInputData, pos),
            sizeof(MorphInputData), positionByteSize, submesh.info.vertexCount);

        if (!baseNormals.empty()) {
            CopyMorphNormalAttributeData(targetData_.data(), baseNormals.data(), baseOffset, sizeof(MorphInputData),
                normalByteSize, submesh.info.vertexCount, baseNormals.size(), 0);
        }

        if (!baseTangents.empty()) {
            CopyMorphTangentAttributeData(targetData_.data(), baseTangents.data(), baseOffset, sizeof(MorphInputData),
                tangentByteSize, submesh.info.vertexCount, baseTangents.size(), 0);
        }

        // Gather non-zero deltas.
        GatherDeltas(submesh, baseOffset, indexOffset, targetSize, targetPositions, targetNormals, targetTangents);

        // Actual buffer size based on the offset and size of the last morph target.
        submesh.morphTargetBufferSize = submesh.morphTargets[submesh.info.morphTargetCount - 1].offset - indexOffset +
                                        submesh.morphTargets[submesh.info.morphTargetCount - 1].byteSize;

        // Clamp to actual size which might be less than what was asked for before gathering the non-zero deltas.
        targetData_.resize(submesh.morphTargetBufferOffset + submesh.morphTargetBufferSize);

        MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
        submeshDesc.morphTargetBuffer.offset = submesh.morphTargetBufferOffset;
        submeshDesc.morphTargetBuffer.byteSize = submesh.morphTargetBufferSize;
        submeshDesc.morphTargetCount = static_cast<uint32_t>(submesh.morphTargets.size());
    }
}

array_view<const uint8_t> MeshBuilder::GetVertexData() const
{
    return array_view<const uint8_t>(vertexData_);
}

array_view<const uint8_t> MeshBuilder::GetJointData() const
{
    return array_view<const uint8_t>(jointData_);
}

array_view<const uint8_t> MeshBuilder::GetIndexData() const
{
    return array_view<const uint8_t>(indexData_);
}

array_view<const uint8_t> MeshBuilder::GetMorphTargetData() const
{
    return array_view<const uint8_t>(targetData_);
}

array_view<const float> MeshBuilder::GetJointBoundsData() const
{
    return array_view(reinterpret_cast<const float*>(jointBoundsData_.data()),
        jointBoundsData_.size() * SIZE_OF_VALUE_TYPE_V<decltype(jointBoundsData_)> / sizeof(float));
}

const array_view<const MeshComponent::Submesh> MeshBuilder::GetSubmeshes() const
{
    return array_view<const MeshComponent::Submesh>(submeshes_);
}

MeshBuilder::BufferEntities MeshBuilder::CreateBuffers(IEcs& ecs) const
{
    BufferEntities entities;
    auto engine = ecs.GetClassFactory().GetInterface<IEngine>();
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    if (!engine || !renderHandleManager) {
        return entities;
    }
    auto rc = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
    if (!rc) {
        return entities;
    }

    auto& em = ecs.GetEntityManager();
    auto& gpuResourceManager = rc->GetDevice().GetGpuResourceManager();

    // Create vertex buffer for this mesh.
    {
        const GpuBufferDesc vertexBufferDesc = GetVertexBufferDesc(vertexData_.size(), !targetData_.empty());
        entities.vertexBuffer =
            CreateBuffer(em, *renderHandleManager, gpuResourceManager, vertexBufferDesc, vertexData_);
    }
    // Create index buffer for this mesh.
    if (!indexData_.empty()) {
        const GpuBufferDesc indexBufferDesc = GetIndexBufferDesc<uint32_t>(indexCount_);
        entities.indexBuffer = CreateBuffer(em, *renderHandleManager, gpuResourceManager, indexBufferDesc, indexData_);
    }

    if (!jointData_.empty()) {
        const GpuBufferDesc jointAttributeDesc = GetVertexBufferDesc(jointData_.size(), false);
        entities.jointBuffer =
            CreateBuffer(em, *renderHandleManager, gpuResourceManager, jointAttributeDesc, jointData_);
    }

    if (!targetData_.empty()) {
        const GpuBufferDesc targetDesc = GetMorphTargetBufferDesc(targetData_.size());
        entities.morphBuffer = CreateBuffer(em, *renderHandleManager, gpuResourceManager, targetDesc, targetData_);
    }
    return entities;
}

Entity MeshBuilder::CreateMesh(IEcs& ecs) const
{
    if (vertexData_.empty()) {
        return {};
    }

    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    if (!meshManager) {
        return {};
    }
    auto& em = ecs.GetEntityManager();
    auto meshEntity = em.Create();
    meshManager->Create(meshEntity);
    if (auto meshHandle = meshManager->Write(meshEntity); meshHandle) {
        MeshComponent& mesh = *meshHandle;

        // Copy skin joint bounding boxes.
        const size_t jointBoundsDataSize = jointBoundsData_.size();
        if (jointBoundsDataSize != 0) {
            mesh.jointBounds.reserve(jointBoundsDataSize * JOINT_BOUNDS_COMPONENTS);
            for (const auto& bounds : jointBoundsData_) {
                for (const auto& f : bounds.min.data) {
                    mesh.jointBounds.push_back(f);
                }
                for (const auto& f : bounds.max.data) {
                    mesh.jointBounds.push_back(f);
                }
            }
        }

        // Copy submeshes for this mesh.
        mesh.submeshes.insert(mesh.submeshes.end(), submeshes_.begin(), submeshes_.end());

        // Create buffers for whole mesh and assign them to submeshes.
        const auto bufferEntities = CreateBuffers(ecs);
        FillSubmeshBuffers(mesh.submeshes, bufferEntities);

        // AABB for mesh
        const auto minMax = CalculateAabb(submeshes_);
        mesh.aabbMin = minMax.minAABB;
        mesh.aabbMax = minMax.maxAABB;
    }
    return meshEntity;
}

const IInterface* MeshBuilder::GetInterface(const Uid& uid) const
{
    if (uid == IMeshBuilder::UID) {
        return this;
    }
    return nullptr;
}

IInterface* MeshBuilder::GetInterface(const Uid& uid)
{
    if (uid == IMeshBuilder::UID) {
        return this;
    }
    return nullptr;
}

void MeshBuilder::Ref()
{
    refCount_++;
}

void MeshBuilder::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}
CORE3D_END_NAMESPACE()
