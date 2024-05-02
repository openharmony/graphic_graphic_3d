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

#include "mesh_builder.h"

#include <algorithm>
#include <securec.h>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/util/intf_picking.h>
#include <base/containers/type_traits.h>
#include <base/math/float_packer.h>
#include <base/math/mathf.h>
#include <base/math/vector_util.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_handle.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "util/mesh_util.h"

namespace {
#include "3d/shaders/common/morph_target_structs.h"
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr uint32_t BUFFER_ALIGN = 0x100; // on Nvidia = 0x20, on Mali and Intel = 0x10, SBO on Mali = 0x100
constexpr auto POSITION_FORMAT = BASE_FORMAT_R32G32B32_SFLOAT;
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
constexpr auto NORMAL_FORMAT = BASE_FORMAT_R16G16B16_SFLOAT;
constexpr auto TANGENT_FORMAT = BASE_FORMAT_R16G16B16A16_SFLOAT;
#else
constexpr auto NORMAL_FORMAT = BASE_FORMAT_R32G32B32_SFLOAT;
constexpr auto TANGENT_FORMAT = BASE_FORMAT_R32G32B32_SFLOAT;
#endif

constexpr auto R = 0;
constexpr auto G = 1;
constexpr auto B = 2;

constexpr auto RG = 2;
constexpr auto RGB = 3;
constexpr auto RGBA = 4;

template<typename Container>
using ContainerValueType = typename remove_reference_t<Container>::value_type;

template<typename Container>
constexpr const auto SIZE_OF_VALUE_TYPE_V = sizeof(ContainerValueType<Container>);

constexpr inline size_t Align(size_t value, size_t align) noexcept
{
    if (align == 0U) {
        return value;
    }

    return ((value + align - 1U) / align) * align;
}

const VertexInputDeclaration::VertexInputAttributeDescription* GetVertexAttributeDescription(uint32_t location,
    const array_view<const VertexInputDeclaration::VertexInputAttributeDescription>& attributeDescriptions) noexcept
{
    const auto cmpLocationIndex = [location](auto const& attribute) { return attribute.location == location; };
    if (const auto pos = std::find_if(attributeDescriptions.begin(), attributeDescriptions.end(), cmpLocationIndex);
        pos != attributeDescriptions.end()) {
        return pos.ptr();
    }

    return nullptr;
}

const VertexInputDeclaration::VertexInputBindingDescription* GetVertexBindingeDescription(uint32_t bindingIndex,
    const array_view<const VertexInputDeclaration::VertexInputBindingDescription>& bindingDescriptions) noexcept
{
    const auto cmpBindingIndex = [bindingIndex](auto const& binding) { return binding.binding == bindingIndex; };
    if (const auto pos = std::find_if(bindingDescriptions.begin(), bindingDescriptions.end(), cmpBindingIndex);
        pos != bindingDescriptions.end()) {
        return pos.ptr();
    }

    return nullptr;
}

struct Intermediate {
    float data[RGBA];
};

using ToIntermediate = float (*)(const uint8_t* src) noexcept;
using FromIntermediate = void (*)(uint8_t* dst, float) noexcept;

struct FormatProperties {
    size_t componentCount;
    size_t componentByteSize;
    Format format;
    bool isNormalized;
    bool isSigned;
    ToIntermediate toIntermediate;
    FromIntermediate fromIntermediate;
};

struct OutputBuffer {
    BASE_NS::Format format;
    uint32_t stride;
    BASE_NS::array_view<uint8_t> buffer;
};

template<typename T, size_t N, size_t M>
inline void GatherMin(T (&minimum)[N], const T (&value)[M])
{
    for (size_t i = 0; i < Math::min(N, M); ++i) {
        minimum[i] = Math::min(minimum[i], value[i]);
    }
}

template<typename T, size_t N, size_t M>
inline void GatherMax(T (&minimum)[N], const T (&value)[M])
{
    for (size_t i = 0; i < Math::min(N, M); ++i) {
        minimum[i] = Math::max(minimum[i], value[i]);
    }
}

// floating point to signed normalized integer
template<typename T, typename = enable_if_t<is_signed_v<T>>>
constexpr inline T Snorm(float v) noexcept
{
    const float round = v >= 0.f ? +.5f : -.5f;
    v = v < -1.f ? -1.f : (v > 1.f ? 1.f : v);
    return static_cast<T>(v * static_cast<float>(std::numeric_limits<T>::max()) + round);
}

// signed normalized integer to floating point
template<typename T, typename = enable_if_t<is_signed_v<T> && is_integral_v<T>>>
constexpr inline float Snorm(T v) noexcept
{
    return static_cast<float>(v) / static_cast<float>(std::numeric_limits<T>::max());
}

// floating point to unsigned normalized integer
template<typename T, typename = enable_if_t<is_unsigned_v<T>>>
constexpr inline T Unorm(float v) noexcept
{
    v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    return static_cast<T>(v * static_cast<float>(std::numeric_limits<T>::max()) + 0.5f);
}

// unsigned normalized integer to floating point
template<typename T, typename = enable_if_t<is_unsigned_v<T> && is_integral_v<T>>>
constexpr inline float Unorm(T v) noexcept
{
    return static_cast<float>(v) / static_cast<float>(std::numeric_limits<T>::max());
}

// floating point to signed integer
template<typename T, typename = enable_if_t<is_signed_v<T>>>
constexpr inline T Sint(float v) noexcept
{
    const float round = v >= 0.f ? +.5f : -.5f;
    constexpr auto l = static_cast<float>(std::numeric_limits<T>::lowest());
    constexpr auto h = static_cast<float>(std::numeric_limits<T>::max());
    v = v < l ? l : (v > h ? h : v);
    return static_cast<T>(v + round);
}

// signed integer to floating point
template<typename T, typename = enable_if_t<is_signed_v<T> && is_integral_v<T>>>
constexpr inline float Sint(T v) noexcept
{
    return static_cast<float>(v);
}

// floating point to unsigned integer
template<typename T, typename = enable_if_t<is_unsigned_v<T>>>
constexpr inline T Uint(float v) noexcept
{
    constexpr auto h = static_cast<float>(std::numeric_limits<T>::max());
    v = v < 0.f ? 0.f : (v > h ? h : v);
    return static_cast<T>(v + 0.5f);
}

// unsigned integer to floating point
template<typename T, typename = enable_if_t<is_unsigned_v<T> && is_integral_v<T>>>
constexpr inline float Uint(T v) noexcept
{
    return static_cast<float>(v);
}

// helpers for ingeter - integer conversions
template<typename T, typename U>
struct IntegerNorm {
    using InType = T;
    using OutType = U;

    static U convert(T v) noexcept
    {
        constexpr auto dstH = std::numeric_limits<U>::max();
        constexpr auto srcH = std::numeric_limits<T>::max();
        if constexpr (is_signed_v<T>) {
            auto round = v >= 0 ? (srcH - 1) : (-srcH + 1);
            return static_cast<U>(((static_cast<int32_t>(v) * dstH) + round) / srcH);
        } else {
            return static_cast<U>(((static_cast<uint32_t>(v) * dstH) + (srcH - 1)) / srcH);
        }
    }
};

template<typename T, typename U>
struct IntegerToInt {
    using InType = T;
    using OutType = U;

    static U convert(T v) noexcept
    {
        return static_cast<U>(v);
    }
};

template<typename Converter, size_t components>
void Convert(uint8_t* dstPtr, size_t dstStride, const uint8_t* srcPtr, size_t srcStride, size_t elements)
{
    while (elements--) {
        for (auto i = 0U; i < components; ++i) {
            reinterpret_cast<typename Converter::OutType*>(dstPtr)[i] = static_cast<typename Converter::OutType>(
                Converter::convert(reinterpret_cast<const typename Converter::InType*>(srcPtr)[i]));
        }
        srcPtr += srcStride;
        dstPtr += dstStride;
    }
}

// helpers for type T - float - type U conversions
template<typename T>
struct Norm {
    using Type = T;

    static T To(float f) noexcept
    {
        if constexpr (is_signed_v<T>) {
            return Snorm<T>(f);
        } else {
            return Unorm<T>(f);
        }
    }

    static float From(T v) noexcept
    {
        if constexpr (is_signed_v<T>) {
            return Snorm<T>(v);
        } else {
            return Unorm<T>(v);
        }
    }
};

template<typename T>
struct Int {
    using Type = T;

    static T To(float f)
    {
        if constexpr (is_signed_v<T>) {
            return Sint<T>(f);
        } else {
            return Uint<T>(f);
        }
    }

    static float From(T v)
    {
        if constexpr (is_signed_v<T>) {
            return Sint<T>(v);
        } else {
            return Uint<T>(v);
        }
    }
};

template<typename T>
struct Float {
    using Type = T;

    static T To(float f)
    {
        if constexpr (is_same_v<T, float>) {
            return f;
        }
        if constexpr (sizeof(T) == sizeof(uint16_t)) {
            return Math::F32ToF16(f);
        }
    }

    static float From(T v)
    {
        if constexpr (is_same_v<T, float>) {
            return v;
        }
        if constexpr (sizeof(T) == sizeof(uint16_t)) {
            return Math::F16ToF32(v);
        }
    }
};

template<typename SourceFn, typename DestFn, size_t components>
void Convert(uint8_t* dstPtr, size_t dstStride, const uint8_t* srcPtr, size_t srcStride, size_t elements)
{
    while (elements--) {
        for (auto i = 0U; i < components; ++i) {
            reinterpret_cast<typename DestFn::Type*>(dstPtr)[i] =
                DestFn::To(SourceFn::From(reinterpret_cast<const typename SourceFn::Type*>(srcPtr)[i]));
        }
        srcPtr += srcStride;
        dstPtr += dstStride;
    }
}

template<typename SourceFn>
float From(const uint8_t* src) noexcept
{
    return SourceFn::From(reinterpret_cast<const typename SourceFn::Type*>(src)[R]);
}

template<typename DestFn>
void To(uint8_t* dst, float f) noexcept
{
    reinterpret_cast<typename DestFn::Type*>(dst)[R] = DestFn::To(f);
}

static constexpr const FormatProperties DATA_FORMATS[] = {
    { 0, 0, BASE_FORMAT_UNDEFINED, false, false, nullptr, nullptr },

    { 1, 1, BASE_FORMAT_R8_UNORM, true, false, From<Norm<uint8_t>>, To<Norm<uint8_t>> },
    { 1, 1, BASE_FORMAT_R8_SNORM, true, true, From<Norm<int8_t>>, To<Norm<int8_t>> },
    { 1, 1, BASE_FORMAT_R8_UINT, false, false, From<Int<uint8_t>>, To<Int<uint8_t>> },

    { 3, 1, BASE_FORMAT_R8G8B8_SNORM, true, false, From<Norm<int8_t>>, To<Norm<int8_t>> },

    { 4, 1, BASE_FORMAT_R8G8B8A8_UNORM, true, false, From<Norm<uint8_t>>, To<Norm<uint8_t>> },
    { 4, 1, BASE_FORMAT_R8G8B8A8_SNORM, true, false, From<Norm<int8_t>>, To<Norm<int8_t>> },
    { 4, 1, BASE_FORMAT_R8G8B8A8_UINT, false, false, From<Int<uint8_t>>, To<Int<uint8_t>> },

    { 1, 2, BASE_FORMAT_R16_UINT, false, false, From<Int<uint16_t>>, To<Int<uint16_t>> },

    { 2, 2, BASE_FORMAT_R16G16_UNORM, true, false, From<Norm<uint16_t>>, To<Norm<uint16_t>> },
    { 2, 2, BASE_FORMAT_R16G16_UINT, false, true, From<Int<uint16_t>>, To<Int<uint16_t>> },
    { 2, 2, BASE_FORMAT_R16G16_SFLOAT, false, true, From<Float<uint16_t>>, To<Float<uint16_t>> },

    { 3, 2, BASE_FORMAT_R16G16B16_UINT, true, true, From<Int<uint16_t>>, To<Int<uint16_t>> },
    { 3, 2, BASE_FORMAT_R16G16B16_SINT, true, true, From<Int<int16_t>>, To<Int<int16_t>> },
    { 3, 2, BASE_FORMAT_R16G16B16_SFLOAT, false, true, From<Float<uint16_t>>, To<Float<uint16_t>> },

    { 4, 2, BASE_FORMAT_R16G16B16A16_UNORM, true, true, From<Norm<uint16_t>>, To<Norm<uint16_t>> },
    { 4, 2, BASE_FORMAT_R16G16B16A16_SNORM, true, true, From<Norm<int16_t>>, To<Norm<int16_t>> },
    { 4, 2, BASE_FORMAT_R16G16B16A16_UINT, false, false, From<Int<uint16_t>>, To<Int<uint16_t>> },
    { 4, 2, BASE_FORMAT_R16G16B16A16_SFLOAT, false, true, From<Float<uint16_t>>, To<Float<uint16_t>> },

    { 1, 4, BASE_FORMAT_R32_UINT, false, false, From<Int<uint32_t>>, To<Int<uint32_t>> },

    { 2, 4, BASE_FORMAT_R32G32_SFLOAT, false, true, From<Float<float>>, To<Float<float>> },

    { 3, 4, BASE_FORMAT_R32G32B32_SFLOAT, false, true, From<Float<float>>, To<Float<float>> },

    { 4, 4, BASE_FORMAT_R32G32B32A32_SFLOAT, false, true, From<Float<float>>, To<Float<float>> },
};

template<class It, class T, class Pred>
constexpr It LowerBound(It first, const It last, const T& val, Pred pred)
{
    auto count = std::distance(first, last);

    while (count > 0) {
        const auto half = count / 2;
        const auto mid = std::next(first, half);
        if (pred(*mid, val)) {
            first = mid + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return first;
}

constexpr const FormatProperties& GetFormatSpec(Format format)
{
#if defined(__cpp_lib_constexpr_algorithms) && (__cpp_lib_constexpr_algorithms >= 201806L)
    static_assert(std::is_sorted(std::begin(DATA_FORMATS), std::end(DATA_FORMATS),
        [](const FormatProperties& lhs, const FormatProperties& rhs) { return lhs.format < rhs.format; }));
#endif
    if (auto pos = LowerBound(std::begin(DATA_FORMATS), std::end(DATA_FORMATS), format,
            [](const FormatProperties& element, Format value) { return element.format < value; });
        (pos != std::end(DATA_FORMATS)) && (pos->format == format)) {
        return *pos;
    }
    return DATA_FORMATS[0];
}

size_t GetVertexAttributeByteSize(
    const uint32_t vertexAttributeLocation, const VertexInputDeclarationView& vertexInputDeclaration)
{
    if (const auto* vertexAttributeDesc =
            GetVertexAttributeDescription(vertexAttributeLocation, vertexInputDeclaration.attributeDescriptions);
        vertexAttributeDesc) {
        const FormatProperties& properties = GetFormatSpec(vertexAttributeDesc->format);

        CORE_ASSERT_MSG(
            properties.format != BASE_FORMAT_UNDEFINED, "Format not supported (%u).", vertexAttributeDesc->format);
        return properties.componentCount * properties.componentByteSize;
    }
    return 0;
}

// For each joint 6 values defining the min and max bounds (world space AABB) of the vertices affected by the joint.
constexpr const size_t JOINT_BOUNDS_COMPONENTS = 6u;

GpuBufferDesc GetVertexBufferDesc(size_t byteSize, BufferUsageFlags additionalFlags, bool morphTarget)
{
    // NOTE: storage buffer usage is currently enabled for all
    // there's no API to define flags for auto loaded and build meshes
    // one might always want more (and e.g. with particle cases we should use texel storage for auto formats)
    GpuBufferDesc desc;
    desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    desc.usageFlags |= additionalFlags;
    desc.engineCreationFlags = 0U;
    // EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_ENABLE_MEMORY_OPTIMIZATIONS;
    if (morphTarget) {
        desc.engineCreationFlags |= CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    }
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.byteSize = (uint32_t)byteSize;

    return desc;
}

template<typename T>
constexpr GpuBufferDesc GetIndexBufferDesc(size_t indexCount, BufferUsageFlags additionalFlags)
{
    // NOTE: storage buffer usage is currently enabled for all
    // there's no API to define flags for auto loaded and build meshes
    // one might always want more (and e.g. with particle cases we should use texel storage for auto formats)
    GpuBufferDesc indexBufferDesc;
    indexBufferDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                 BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    indexBufferDesc.usageFlags |= additionalFlags;
    indexBufferDesc.engineCreationFlags = 0;
    indexBufferDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    indexBufferDesc.byteSize = sizeof(T) * (uint32_t)indexCount;

    return indexBufferDesc;
}

constexpr GpuBufferDesc GetMorphTargetBufferDesc(size_t byteSize, BufferUsageFlags additionalFlags)
{
    // NOTE: These are only morph targets which are read only
    GpuBufferDesc desc;
    desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    desc.usageFlags |= additionalFlags;
    desc.engineCreationFlags = 0; // read-only
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.byteSize = (uint32_t)byteSize;

    return desc;
}

EntityReference CreateBuffer(IEntityManager& entityManager, IRenderHandleComponentManager& handleManager,
    const RenderHandleReference& bufferHandle)
{
    auto sharedEntity = entityManager.CreateReferenceCounted();
    handleManager.Create(sharedEntity);
    handleManager.Write(sharedEntity)->reference = bufferHandle;
    return sharedEntity;
}

MeshBuilder::BufferHandles CreateGpuBuffers(IRenderContext& renderContext, size_t vertexDataSize, size_t indexDataSize,
    uint32_t indexCount, size_t jointDataSize, size_t targetDataSize,
    const MeshBuilder::GpuBufferCreateInfo& createInfo)
{
    MeshBuilder::BufferHandles handles;

    auto& gpuResourceManager = renderContext.GetDevice().GetGpuResourceManager();

    {
        // targetDataSize is zero when there's no morph targets
        const GpuBufferDesc vertexBufferDesc =
            GetVertexBufferDesc(vertexDataSize, createInfo.vertexBufferFlags, targetDataSize != 0U);
        handles.vertexBuffer = gpuResourceManager.Create(vertexBufferDesc);
    }

    if (indexDataSize) {
        const GpuBufferDesc indexBufferDesc = GetIndexBufferDesc<uint32_t>(indexCount, createInfo.indexBufferFlags);
        handles.indexBuffer = gpuResourceManager.Create(indexBufferDesc);
    }

    if (jointDataSize) {
        const GpuBufferDesc jointAttributeDesc =
            GetVertexBufferDesc(jointDataSize, createInfo.vertexBufferFlags, false);
        handles.jointBuffer = gpuResourceManager.Create(jointAttributeDesc);
    }

    if (targetDataSize) {
        const GpuBufferDesc targetDesc = GetMorphTargetBufferDesc(targetDataSize, createInfo.morphBufferFlags);
        handles.morphBuffer = gpuResourceManager.Create(targetDesc);
    }

    return handles;
}

void StageToBuffers(IRenderContext& renderContext, size_t vertexDataSize, size_t indexDataSize, size_t jointDataSize,
    size_t targetDataSize, const MeshBuilder::BufferHandles& handles, const RenderHandleReference& stagingBuffer)
{
    constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
    auto staging = static_cast<IRenderDataStoreDefaultStaging*>(
        renderContext.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING.data()));
    BufferCopy copyData {};
    if (vertexDataSize) {
        copyData.size = static_cast<uint32_t>(vertexDataSize);
        staging->CopyBufferToBuffer(stagingBuffer, handles.vertexBuffer, copyData);
        copyData.srcOffset += copyData.size;
    }
    if (indexDataSize) {
        copyData.size = static_cast<uint32_t>(indexDataSize);
        staging->CopyBufferToBuffer(stagingBuffer, handles.indexBuffer, copyData);
        copyData.srcOffset += copyData.size;
    }

    if (jointDataSize) {
        copyData.size = static_cast<uint32_t>(jointDataSize);
        staging->CopyBufferToBuffer(stagingBuffer, handles.jointBuffer, copyData);
        copyData.srcOffset += copyData.size;
    }
    if (targetDataSize) {
        copyData.size = static_cast<uint32_t>(targetDataSize);
        staging->CopyBufferToBuffer(stagingBuffer, handles.morphBuffer, copyData);
    }
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

void Fill(OutputBuffer& dstData, const MeshBuilder::DataBuffer& srcData, size_t count)
{
    if (!count) {
        return;
    }
    const auto dstFormat = GetFormatSpec(dstData.format);
    if (dstFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("destination format (%u) not supported", dstData.format);
        return;
    }
    const auto dstElementSize = dstFormat.componentCount * dstFormat.componentByteSize;
    if (count == 0) {
        return;
    }
    if ((dstElementSize > dstData.stride) || (dstData.stride > (dstData.buffer.size() / count))) {
        return;
    }
    const auto srcFormat = GetFormatSpec(srcData.format);
    if (srcFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("source format (%u) not supported", srcData.format);
        return;
    }
    const auto srcElementSize = srcFormat.componentCount * srcFormat.componentByteSize;
    if ((srcElementSize > srcData.stride) || (srcData.stride > (srcData.buffer.size() / count))) {
        return;
    }
    if (dstData.format == srcData.format) {
        // no conversion required
        if (dstData.stride == srcData.stride && dstData.stride == dstElementSize) {
            // strides match and no padding
            CloneData(dstData.buffer.data(), dstData.buffer.size(), srcData.buffer.data(), srcElementSize * count);
        } else {
            // stride mismatch or padding
            auto dstPtr = dstData.buffer.data();
            auto dstSize = dstData.buffer.size();
            auto srcPtr = srcData.buffer.data();
            while (count--) {
                CloneData(dstPtr, dstSize, srcPtr, srcElementSize);
                dstPtr += dstData.stride;
                srcPtr += srcData.stride;
                dstSize -= dstData.stride;
            }
        }
    } else if (!srcFormat.toIntermediate || !dstFormat.fromIntermediate) {
        CORE_LOG_E("missing conversion from %u to %u", srcFormat.format, dstFormat.format);
    } else {
        // must convert between formats
        auto dstPtr = dstData.buffer.data();
        auto srcPtr = srcData.buffer.data();
        // attempt to inline commonly used conversions
        switch (srcData.format) {
            case BASE_FORMAT_R8_UINT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16_UINT:
                        Convert<IntegerToInt<uint8_t, uint16_t>, 1U>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R8G8B8_SNORM: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16B16A16_SNORM:
                        Convert<IntegerNorm<int8_t, int16_t>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    case BASE_FORMAT_R16G16B16_SFLOAT:
                        Convert<Norm<int8_t>, Float<uint16_t>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    case BASE_FORMAT_R32G32B32_SFLOAT:
                        Convert<Norm<int8_t>, Float<float>, RGB>(dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R8G8B8A8_SNORM: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16B16A16_SNORM:
                        Convert<IntegerNorm<int8_t, int16_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16_UNORM: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16_SFLOAT:
                        Convert<Norm<uint16_t>, Float<uint16_t>, RG>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16_UINT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16_SFLOAT:
                        Convert<Int<uint16_t>, Float<uint16_t>, RG>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16_SFLOAT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R32G32_SFLOAT:
                        Convert<Float<uint16_t>, Float<float>, RG>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16B16_UINT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R32G32B32_SFLOAT:
                        Convert<Int<uint16_t>, Float<float>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16B16A16_UNORM: {
                switch (dstData.format) {
                    case BASE_FORMAT_R8G8B8A8_UNORM:
                        Convert<IntegerNorm<uint16_t, uint8_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }

            } break;

            case BASE_FORMAT_R16G16B16A16_SNORM: {
                switch (dstData.format) {
                    case BASE_FORMAT_R32G32B32_SFLOAT:
                        Convert<Norm<int16_t>, Float<float>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }

            } break;

            case BASE_FORMAT_R16G16B16A16_UINT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R8G8B8A8_UINT:
                        Convert<IntegerToInt<uint16_t, uint8_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R16G16B16A16_SFLOAT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16B16A16_SNORM:
                        Convert<Float<uint16_t>, Norm<int16_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R32_UINT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16_UINT:
                        Convert<IntegerToInt<uint32_t, uint16_t>, 1U>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }

            } break;

            case BASE_FORMAT_R32G32_SFLOAT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16_SFLOAT:
                        Convert<Float<float>, Float<uint16_t>, RG>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R32G32B32_SFLOAT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R16G16B16_SFLOAT:
                        Convert<Float<float>, Float<uint16_t>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    case BASE_FORMAT_R16G16B16A16_SNORM:
                        Convert<Float<float>, Norm<int16_t>, RGB>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            case BASE_FORMAT_R32G32B32A32_SFLOAT: {
                switch (dstData.format) {
                    case BASE_FORMAT_R8G8B8A8_UNORM:
                        Convert<Float<float>, Norm<uint8_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    case BASE_FORMAT_R16G16B16A16_SNORM:
                        Convert<Float<float>, Norm<int16_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    case BASE_FORMAT_R16G16B16A16_SFLOAT:
                        Convert<Float<float>, Float<uint16_t>, RGBA>(
                            dstPtr, dstData.stride, srcPtr, srcData.stride, count);
                        return;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }

        CORE_LOG_V("%u %u", srcData.format, dstData.format);
        const auto components = Math::min(srcFormat.componentCount, dstFormat.componentCount);
        while (count--) {
            for (auto i = 0U; i < components; ++i) {
                dstFormat.fromIntermediate(dstPtr + i * dstFormat.componentByteSize,
                    srcFormat.toIntermediate(srcPtr + i * srcFormat.componentByteSize));
            }
            auto intermediate = srcFormat.toIntermediate(srcPtr);
            dstFormat.fromIntermediate(dstPtr, intermediate);
            srcPtr += srcData.stride;
            dstPtr += dstData.stride;
        }
    }
}

template<typename T>
void SmoothNormal(array_view<const T> indices, const Math::Vec3* posPtr, Math::Vec3* norPtr)
{
    for (auto i = 0U; i < indices.size(); i += 3) { // 3: step
        const auto aa = indices[i];
        const auto bb = indices[i + 1];
        const auto cc = indices[i + 2]; // 2: index
        const auto& pos1 = posPtr[aa];
        const auto& pos2 = posPtr[bb];
        const auto& pos3 = posPtr[cc];
        auto faceNorm = Math::Cross(pos2 - pos1, pos3 - pos1);
        norPtr[aa] += faceNorm;
        norPtr[bb] += faceNorm;
        norPtr[cc] += faceNorm;
    }
}

void GenerateDefaultNormals(vector<uint8_t>& generatedNormals, const IMeshBuilder::DataBuffer& indices,
    const IMeshBuilder::DataBuffer& positions, uint32_t vertexCount)
{
    auto offset = generatedNormals.size();
    generatedNormals.resize(generatedNormals.size() + sizeof(Math::Vec3) * vertexCount);
    auto* norPtr = reinterpret_cast<Math::Vec3*>(generatedNormals.data() + offset);
    auto* posPtr = reinterpret_cast<const Math::Vec3*>(positions.buffer.data());
    if (indices.buffer.empty()) {
        // Mesh without indices will have flat normals
        for (auto i = 0U; i < vertexCount; i += 3) { // 3: step
            const auto& pos1 = posPtr[i];
            const auto& pos2 = posPtr[i + 1];
            const auto& pos3 = posPtr[i + 2]; // 2: index
            auto faceNorm = Math::Normalize(Math::Cross(pos2 - pos1, pos3 - pos1));
            norPtr[i] = faceNorm;
            norPtr[i + 1] = faceNorm;
            norPtr[i + 2] = faceNorm; // 2: index
        }
    } else {
        // With indexed data flat normals would require duplicating shared vertices. Instead calculate smooth normals.
        if (indices.stride == sizeof(uint16_t)) {
            auto view = array_view(
                reinterpret_cast<const uint16_t*>(indices.buffer.data()), indices.buffer.size() / indices.stride);
            SmoothNormal(view, posPtr, norPtr);
        } else if (indices.stride == sizeof(uint32_t)) {
            auto view = array_view(
                reinterpret_cast<const uint32_t*>(indices.buffer.data()), indices.buffer.size() / indices.stride);
            SmoothNormal(view, posPtr, norPtr);
        }
        for (auto& nor : array_view(norPtr, vertexCount)) {
            nor = Math::Normalize(nor);
        }
    }
}

void GenerateDefaultUvs(vector<uint8_t>& generatedUvs, uint32_t vertexCount)
{
    auto offset = generatedUvs.size();
    generatedUvs.resize(generatedUvs.size() + sizeof(Math::Vec2) * vertexCount);
    auto* ptr = reinterpret_cast<Math::Vec2*>(generatedUvs.data() + offset);
    std::fill(ptr, ptr + vertexCount, Math::Vec2(0.0f, 0.0f));
}

void GenerateDefaultTangents(IMeshBuilder::DataBuffer& tangents, vector<uint8_t>& generatedTangents,
    const IMeshBuilder::DataBuffer& indices, const IMeshBuilder::DataBuffer& positions,
    const IMeshBuilder::DataBuffer& normals, const IMeshBuilder::DataBuffer& uvs, uint32_t vertexCount)
{
    auto offset = generatedTangents.size();
    generatedTangents.resize(generatedTangents.size() + sizeof(Math::Vec4) * vertexCount);
    tangents.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    tangents.stride = sizeof(Math::Vec4);
    tangents.buffer = generatedTangents;

    auto posView = array_view(reinterpret_cast<const Math::Vec3*>(positions.buffer.data()), vertexCount);
    auto norView = array_view(reinterpret_cast<const Math::Vec3*>(normals.buffer.data()), vertexCount);
    auto uvsView = array_view(reinterpret_cast<const Math::Vec2*>(uvs.buffer.data()), vertexCount);

    auto outTangents = array_view(reinterpret_cast<Math::Vec4*>(generatedTangents.data() + offset), vertexCount);

    vector<uint8_t> indexData(indices.buffer.size());

    const auto indexCountCount = indices.buffer.size() / indices.stride;
    switch (indices.stride) {
        case sizeof(uint8_t): {
            auto indicesView = array_view(reinterpret_cast<const uint8_t*>(indices.buffer.data()), indexCountCount);
            MeshUtil::CalculateTangents(indicesView, posView, norView, uvsView, outTangents);
            break;
        }
        case sizeof(uint16_t): {
            auto indicesView = array_view(reinterpret_cast<const uint16_t*>(indices.buffer.data()), indexCountCount);
            MeshUtil::CalculateTangents(indicesView, posView, norView, uvsView, outTangents);
            break;
        }
        case sizeof(uint32_t): {
            auto indicesView = array_view(reinterpret_cast<const uint32_t*>(indices.buffer.data()), indexCountCount);
            MeshUtil::CalculateTangents(indicesView, posView, norView, uvsView, outTangents);
            break;
        }
        default:
            CORE_ASSERT_MSG(false, "Invalid elementSize %u", indices.stride);
    }
}
} // namespace

MeshBuilder::MeshBuilder(IRenderContext& renderContext) : renderContext_(renderContext) {}

// Public interface from IMeshBuilder
void MeshBuilder::Initialize(const VertexInputDeclarationView& vertexInputDeclaration, size_t submeshCount)
{
    submeshInfos_.clear();
    submeshInfos_.reserve(submeshCount);
    submeshes_.clear();
    submeshes_.resize(submeshCount);

    vertexCount_ = 0;
    indexCount_ = 0;

    vertexDataSize_ = 0;
    indexDataSize_ = 0;
    jointDataSize_ = 0;
    targetDataSize_ = 0;

    jointBoundsData_.clear();

    bufferHandles_ = {};
    if (stagingPtr_) {
        stagingPtr_ = nullptr;
        auto& gpuResourceManager = renderContext_.GetDevice().GetGpuResourceManager();
        gpuResourceManager.UnmapBuffer(stagingBuffer_);
    }
    stagingBuffer_ = {};
    vertexInputDeclaration_ = vertexInputDeclaration;
}

void MeshBuilder::AddSubmesh(const Submesh& info)
{
    submeshInfos_.push_back(SubmeshExt { info, {}, {}, {} });
}

const MeshBuilder::Submesh& MeshBuilder::GetSubmesh(size_t index) const
{
    return submeshInfos_[index].info;
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

    vertexDataSize_ = vertexBufferSizeInBytes;
    indexDataSize_ = bufferSizes.indexBuffer;
    jointDataSize_ = bufferSizes.jointBuffer;
    targetDataSize_ = bufferSizes.morphVertexData;

    auto& gpuResourceManager = renderContext_.GetDevice().GetGpuResourceManager();

    GpuBufferDesc gpuBufferDesc;
    gpuBufferDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
    gpuBufferDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gpuBufferDesc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                        EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER |
                                        EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
    gpuBufferDesc.byteSize = static_cast<uint32_t>(vertexDataSize_ + indexDataSize_ + jointDataSize_ + targetDataSize_);
    if (gpuBufferDesc.byteSize) {
        stagingBuffer_ = gpuResourceManager.Create(gpuBufferDesc);
        stagingPtr_ = static_cast<uint8_t*>(gpuResourceManager.MapBufferMemory(stagingBuffer_));
    }
}

MeshBuilder::BufferSizesInBytes MeshBuilder::CalculateSizes()
{
    BufferSizesInBytes bufferSizes {};

    const size_t jointIndexSizeInBytes =
        GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOI, vertexInputDeclaration_);
    const size_t jointWeightSizeInBytes =
        GetVertexAttributeByteSize(MeshComponent::Submesh::DM_VB_JOW, vertexInputDeclaration_);

    for (auto& submesh : submeshInfos_) {
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

void MeshBuilder::SetVertexData(size_t submeshIndex, const DataBuffer& positions, const DataBuffer& normals,
    const DataBuffer& texcoords0, const DataBuffer& texcoords1, const DataBuffer& tangents, const DataBuffer& colors)
{
    if (auto buffer = stagingPtr_; buffer) {
        // *Vertex data* | index data | joint data | morph data
        SubmeshExt& submesh = submeshInfos_[submeshIndex];

        // Submesh info for this submesh.
        MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];

        submeshDesc.material = submesh.info.material;
        submeshDesc.vertexCount = submesh.info.vertexCount;
        submeshDesc.instanceCount = submesh.info.instanceCount;

        // If we need to generate tangents we need float copies of position, normal and uv0
        const bool generateTangents = submesh.info.tangents && tangents.buffer.empty();

        // Process position.
        {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_POS];
            WriteData(positions, submesh, MeshComponent::Submesh::DM_VB_POS, acc.offset, acc.byteSize, buffer);
            if (normals.buffer.empty() || generateTangents) {
                auto offset = vertexData_.size();
                vertexData_.resize(offset + sizeof(Math::Vec3) * submeshDesc.vertexCount);
                OutputBuffer dst { BASE_FORMAT_R32G32B32_SFLOAT, sizeof(Math::Vec3),
                    { vertexData_.data() + offset, sizeof(Math::Vec3) * submeshDesc.vertexCount } };
                Fill(dst, positions, submeshDesc.vertexCount);
                submesh.positionOffset = static_cast<int32_t>(offset);
                submesh.positionSize = sizeof(Math::Vec3) * submeshDesc.vertexCount;
            }
        }

        // Process normal.
        if (!normals.buffer.empty()) {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_NOR];
            WriteData(normals, submesh, MeshComponent::Submesh::DM_VB_NOR, acc.offset, acc.byteSize, buffer);
            submesh.hasNormals = true;
            if (generateTangents) {
                auto offset = vertexData_.size();
                vertexData_.resize(offset + sizeof(Math::Vec3) * submeshDesc.vertexCount);
                OutputBuffer dst { BASE_FORMAT_R32G32B32_SFLOAT, sizeof(Math::Vec3),
                    { vertexData_.data() + offset, sizeof(Math::Vec3) * submeshDesc.vertexCount } };
                Fill(dst, normals, submeshDesc.vertexCount);
                submesh.normalOffset = static_cast<int32_t>(offset);
                submesh.normalSize = sizeof(Math::Vec3) * submeshDesc.vertexCount;
            }
        }

        // Process uv.
        if (!texcoords0.buffer.empty()) {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV0];
            WriteData(texcoords0, submesh, MeshComponent::Submesh::DM_VB_UV0, acc.offset, acc.byteSize, buffer);
            submesh.hasUv0 = true;
            if (generateTangents) {
                auto offset = vertexData_.size();
                vertexData_.resize(offset + sizeof(Math::Vec2) * submeshDesc.vertexCount);
                OutputBuffer dst { BASE_FORMAT_R32G32_SFLOAT, sizeof(Math::Vec2),
                    { vertexData_.data() + offset, sizeof(Math::Vec2) * submeshDesc.vertexCount } };
                Fill(dst, texcoords0, submeshDesc.vertexCount);
                submesh.uvOffset = static_cast<int32_t>(offset);
                submesh.uvSize = sizeof(Math::Vec2) * submeshDesc.vertexCount;
            }
        }

        if (!texcoords1.buffer.empty()) {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV1];
            if (WriteData(texcoords1, submesh, MeshComponent::Submesh::DM_VB_UV1, acc.offset, acc.byteSize, buffer)) {
                submeshDesc.flags |= MeshComponent::Submesh::FlagBits::SECOND_TEXCOORD_BIT;
            }
        } else {
            const auto& uv0 = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV0];
            auto& uv1 = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV1];
            uv1 = uv0;
        }

        // Process tangent.
        if (!tangents.buffer.empty() && submesh.info.tangents) {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_TAN];
            if (WriteData(tangents, submesh, MeshComponent::Submesh::DM_VB_TAN, acc.offset, acc.byteSize, buffer)) {
                submeshDesc.flags |= MeshComponent::Submesh::FlagBits::TANGENTS_BIT;
                submesh.hasTangents = true;
            }
        }

        // Process vertex colors.
        if (!colors.buffer.empty() && submesh.info.colors) {
            auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_COL];
            if (WriteData(colors, submesh, MeshComponent::Submesh::DM_VB_COL, acc.offset, acc.byteSize, buffer)) {
                submeshDesc.flags |= MeshComponent::Submesh::FlagBits::VERTEX_COLORS_BIT;
            }
        }
    }
}

void MeshBuilder::SetIndexData(size_t submeshIndex, const DataBuffer& indices)
{
    if (auto buffer = stagingPtr_; buffer) {
        // Vertex data | *index data* | joint data | morph data
        buffer += vertexDataSize_;
        MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
        SubmeshExt& submesh = submeshInfos_[submeshIndex];

        OutputBuffer output { (submesh.info.indexType == IndexType::CORE_INDEX_TYPE_UINT16) ? BASE_FORMAT_R16_UINT
                                                                                            : BASE_FORMAT_R32_UINT,
            static_cast<uint32_t>(
                (submesh.info.indexType == IndexType::CORE_INDEX_TYPE_UINT16) ? sizeof(uint16_t) : sizeof(uint32_t)),
            {} };
        // If we need to generate normals or tangents we need CPU copies of the indices
        if (!submesh.hasNormals || (submesh.info.tangents && !submesh.hasTangents)) {
            // First convert and write to a plain vector
            auto offset = indexData_.size();
            indexData_.resize(offset + output.stride * submesh.info.indexCount);
            output.buffer = { indexData_.data() + offset, output.stride * submesh.info.indexCount };
            Fill(output, indices, submesh.info.indexCount);
            submesh.indexOffset = static_cast<int32_t>(offset);
            submesh.indexSize = output.stride * submesh.info.indexCount;
            // Then copy the data to staging buffer.
            std::copy(indexData_.data() + offset, indexData_.data() + offset + output.stride * submesh.info.indexCount,
                buffer + submesh.indexBufferOffset);
        } else {
            // Convert directly to the staging buffer.
            output.buffer = { buffer + submesh.indexBufferOffset, output.stride * submesh.info.indexCount };
            Fill(output, indices, submesh.info.indexCount);
        }

        submeshDesc.indexCount = submesh.info.indexCount;
        submeshDesc.indexBuffer.indexType = submesh.info.indexType;
        submeshDesc.indexBuffer.offset = submesh.indexBufferOffset;
        submeshDesc.indexBuffer.byteSize = static_cast<uint32_t>(output.buffer.size());
    }
}

void MeshBuilder::SetJointData(
    size_t submeshIndex, const DataBuffer& jointData, const DataBuffer& weightData, const DataBuffer& vertexPositions)
{
    if (auto buffer = stagingPtr_; buffer) {
        // Vertex data | index data | *joint data* | morph data
        buffer += vertexDataSize_ + indexDataSize_;
        MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
        const SubmeshExt& submesh = submeshInfos_[submeshIndex];
        if (const auto* indexAttributeDesc = GetVertexAttributeDescription(
                MeshComponent::Submesh::DM_VB_JOI, vertexInputDeclaration_.attributeDescriptions);
            indexAttributeDesc) {
            if (const VertexInputDeclaration::VertexInputBindingDescription* bindingDesc = GetVertexBindingeDescription(
                    indexAttributeDesc->binding, vertexInputDeclaration_.bindingDescriptions);
                bindingDesc) {
                auto& jointIndexAcc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_JOI];
                jointIndexAcc.offset = (uint32_t)Align(submesh.jointBufferOffset, BUFFER_ALIGN);
                jointIndexAcc.byteSize = (uint32_t)bindingDesc->stride * submesh.info.vertexCount;

                OutputBuffer dstData { indexAttributeDesc->format, bindingDesc->stride,
                    { buffer + jointIndexAcc.offset, jointIndexAcc.byteSize } };
                Fill(dstData, jointData, submesh.info.vertexCount);
            }
        }
        if (const auto* weightAttributeDesc = GetVertexAttributeDescription(
                MeshComponent::Submesh::DM_VB_JOW, vertexInputDeclaration_.attributeDescriptions);
            weightAttributeDesc) {
            if (const VertexInputDeclaration::VertexInputBindingDescription* bindingDesc = GetVertexBindingeDescription(
                    weightAttributeDesc->binding, vertexInputDeclaration_.bindingDescriptions);
                bindingDesc) {
                auto& jointIndexAcc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_JOI];
                // Process joint weights.
                auto& jointWeightAcc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_JOW];
                // index aligned offset + index bytesize -> aligned to offset
                jointWeightAcc.offset = (uint32_t)Align(jointIndexAcc.offset + jointIndexAcc.byteSize, BUFFER_ALIGN);
                jointWeightAcc.byteSize = (uint32_t)bindingDesc->stride * submesh.info.vertexCount;

                OutputBuffer dstData { weightAttributeDesc->format, bindingDesc->stride,
                    { buffer + jointWeightAcc.offset, jointWeightAcc.byteSize } };
                Fill(dstData, weightData, submesh.info.vertexCount);
            }
        }
        submeshDesc.flags |= MeshComponent::Submesh::FlagBits::SKIN_BIT;
        CalculateJointBounds(jointData, weightData, vertexPositions);
    }
}

void MeshBuilder::SetMorphTargetData(size_t submeshIndex, const DataBuffer& basePositions,
    const DataBuffer& baseNormals, const DataBuffer& baseTangents, const DataBuffer& targetPositions,
    const DataBuffer& targetNormals, const DataBuffer& targetTangents)
{
    // Submesh info for this submesh.
    SubmeshExt& submesh = submeshInfos_[submeshIndex];
    if (submesh.info.morphTargetCount > 0) {
        if (auto buffer = stagingPtr_; buffer) {
            // Vertex data | index data | joint data | *morph data*
            buffer += vertexDataSize_ + indexDataSize_ + jointDataSize_;

            // Offset to morph index data is previous offset + size (or zero for the first submesh)
            uint32_t indexOffset = 0u;
            if (submeshIndex) {
                indexOffset = static_cast<uint32_t>(Align(submeshInfos_[submeshIndex - 1u].morphTargetBufferOffset +
                                                              submeshInfos_[submeshIndex - 1u].morphTargetBufferSize,
                    BUFFER_ALIGN));
            }
            submesh.morphTargetBufferOffset = indexOffset;

            // 32bit index/offset for each vertex in each morph target
            const uint32_t indexSize = sizeof(uint32_t) * submesh.info.vertexCount;
            const uint32_t totalIndexSize =
                static_cast<uint32_t>(Align(indexSize * submesh.info.morphTargetCount, BUFFER_ALIGN));

            // Data struct (pos, nor, tan) for each vertex. total amount is target size for each target data and one
            // base data
            const uint32_t targetSize = submesh.info.vertexCount * sizeof(MorphInputData);

            // Base data starts after index data
            const uint32_t baseOffset = indexOffset + totalIndexSize;
            {
                OutputBuffer dstData { POSITION_FORMAT, sizeof(MorphInputData), { buffer + baseOffset, targetSize } };
                Fill(dstData, basePositions, submesh.info.vertexCount);
            }

            if (!baseNormals.buffer.empty()) {
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                OutputBuffer dstData { NORMAL_FORMAT, sizeof(MorphInputData),
                    { buffer + baseOffset + offsetof(MorphInputData, nortan), targetSize } };
#else
                OutputBuffer dstData { NORMAL_FORMAT, sizeof(MorphInputData),
                    { buffer + baseOffset + offsetof(MorphInputData, nor), targetSize } };
#endif
                Fill(dstData, baseNormals, submesh.info.vertexCount);
            }
            if (!baseTangents.buffer.empty()) {
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                OutputBuffer dstData { TANGENT_FORMAT, sizeof(MorphInputData),
                    { buffer + baseOffset + offsetof(MorphInputData, nortan) + 8U, targetSize } };
#else
                OutputBuffer dstData { TANGENT_FORMAT, sizeof(MorphInputData),
                    { buffer + baseOffset + offsetof(MorphInputData, tan), targetSize } };
#endif
                Fill(dstData, baseTangents, submesh.info.vertexCount);
            }
            // Gather non-zero deltas.
            if (targetNormals.buffer.empty() && targetTangents.buffer.empty()) {
                GatherDeltasP(submesh, buffer, baseOffset, indexOffset, targetSize, targetPositions);
            } else if (!targetNormals.buffer.empty() && targetTangents.buffer.empty()) {
                GatherDeltasPN(submesh, buffer, baseOffset, indexOffset, targetSize, targetPositions, targetNormals);
            } else if (targetNormals.buffer.empty() && !targetTangents.buffer.empty()) {
                GatherDeltasPT(submesh, buffer, baseOffset, indexOffset, targetSize, targetPositions, targetTangents);
            } else {
                GatherDeltasPNT(submesh, buffer, baseOffset, indexOffset, targetSize, targetPositions, targetNormals,
                    targetTangents);
            }

            // Actual buffer size based on the offset and size of the last morph target.
            submesh.morphTargetBufferSize = submesh.morphTargets[submesh.info.morphTargetCount - 1].offset -
                                            indexOffset +
                                            submesh.morphTargets[submesh.info.morphTargetCount - 1].byteSize;

            // Clamp to actual size which might be less than what was asked for before gathering the non-zero deltas.
            targetDataSize_ = static_cast<size_t>(submesh.morphTargetBufferOffset + submesh.morphTargetBufferSize);

            MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
            submeshDesc.morphTargetBuffer.offset = submesh.morphTargetBufferOffset;
            submeshDesc.morphTargetBuffer.byteSize = submesh.morphTargetBufferSize;
            submeshDesc.morphTargetCount = static_cast<uint32_t>(submesh.morphTargets.size());
        }
    }
}

void MeshBuilder::SetAABB(size_t submeshIndex, const Math::Vec3& min, const Math::Vec3& max)
{
    MeshComponent::Submesh& submeshDesc = submeshes_[submeshIndex];
    submeshDesc.aabbMin = min;
    submeshDesc.aabbMax = max;
}

void MeshBuilder::CalculateAABB(size_t submeshIndex, const DataBuffer& positions)
{
    const auto posFormat = GetFormatSpec(positions.format);
    if (posFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    const auto posElementSize = posFormat.componentCount * posFormat.componentByteSize;
    if (posElementSize > positions.stride) {
        return;
    }
    auto count = positions.buffer.size() / positions.stride;

    constexpr float maxLimits = std::numeric_limits<float>::max();
    constexpr float minLimits = -std::numeric_limits<float>::max();

    Math::Vec3 finalMinimum = { maxLimits, maxLimits, maxLimits };
    Math::Vec3 finalMaximum = { minLimits, minLimits, minLimits };

    auto srcPtr = positions.buffer.data();
    switch (posFormat.format) {
        case BASE_FORMAT_R16G16_UNORM: {
            uint16_t minimum[RG] { std::numeric_limits<uint16_t>::max() };
            uint16_t maximum[RG] { std::numeric_limits<uint16_t>::lowest() };
            while (count--) {
                uint16_t value[RG] = { reinterpret_cast<const uint16_t*>(srcPtr)[R],
                    reinterpret_cast<const uint16_t*>(srcPtr)[G] };
                srcPtr += positions.stride;
                GatherMin(minimum, value);
                GatherMax(maximum, value);
            }
            for (auto i = 0U; i < RG; ++i) {
                finalMinimum[i] = Norm<uint16_t>::From(minimum[i]);
            }
            finalMinimum[B] = 0.f;
            for (auto i = 0U; i < RG; ++i) {
                finalMaximum[i] = Norm<uint16_t>::From(maximum[i]);
            }
            finalMaximum[B] = 0.f;
        } break;

        case BASE_FORMAT_R8G8B8_SNORM: {
            int8_t minimum[RGB] { std::numeric_limits<int8_t>::max() };
            int8_t maximum[RGB] { std::numeric_limits<int8_t>::lowest() };
            while (count--) {
                int8_t value[RGB] = { reinterpret_cast<const int8_t*>(srcPtr)[R],
                    reinterpret_cast<const int8_t*>(srcPtr)[G], reinterpret_cast<const int8_t*>(srcPtr)[B] };
                srcPtr += positions.stride;
                GatherMin(minimum, value);
                GatherMax(maximum, value);
            }
            for (auto i = 0U; i < RGB; ++i) {
                finalMinimum[i] = Norm<int8_t>::From(minimum[i]);
            }
            for (auto i = 0U; i < RGB; ++i) {
                finalMaximum[i] = Norm<int8_t>::From(maximum[i]);
            }
        } break;

        case BASE_FORMAT_R16G16B16_UINT:
        case BASE_FORMAT_R16G16B16A16_UINT: {
            uint16_t minimum[RGB] { std::numeric_limits<uint16_t>::max() };
            uint16_t maximum[RGB] { std::numeric_limits<uint16_t>::lowest() };
            while (count--) {
                uint16_t value[RGB] = { reinterpret_cast<const uint16_t*>(srcPtr)[R],
                    reinterpret_cast<const uint16_t*>(srcPtr)[G], reinterpret_cast<const uint16_t*>(srcPtr)[B] };
                srcPtr += positions.stride;
                GatherMin(minimum, value);
                GatherMax(maximum, value);
            }
            for (auto i = 0U; i < RGB; ++i) {
                finalMinimum[i] = Int<uint16_t>::From(minimum[i]);
            }
            for (auto i = 0U; i < RGB; ++i) {
                finalMaximum[i] = Int<uint16_t>::From(maximum[i]);
            }
        } break;

        case BASE_FORMAT_R32G32_SFLOAT: {
            while (count--) {
                float value[RG] = { reinterpret_cast<const float*>(srcPtr)[R],
                    reinterpret_cast<const float*>(srcPtr)[G] };
                srcPtr += positions.stride;
                GatherMin(finalMinimum.data, value);
                GatherMax(finalMaximum.data, value);
            }
        } break;

        case BASE_FORMAT_R32G32B32_SFLOAT:
        case BASE_FORMAT_R32G32B32A32_SFLOAT: {
            while (count--) {
                float value[RGB] = { reinterpret_cast<const float*>(srcPtr)[R],
                    reinterpret_cast<const float*>(srcPtr)[G], reinterpret_cast<const float*>(srcPtr)[B] };
                srcPtr += positions.stride;
                GatherMin(finalMinimum.data, value);
                GatherMax(finalMaximum.data, value);
            }
        } break;

        default:
            CORE_LOG_W("CalculateAABB: position format %u not handled.", posFormat.format);
            break;
    }
    SetAABB(submeshIndex, finalMinimum, finalMaximum);
}

array_view<const uint8_t> MeshBuilder::GetVertexData() const
{
    return array_view<const uint8_t>(stagingPtr_, vertexDataSize_);
}

array_view<const uint8_t> MeshBuilder::GetIndexData() const
{
    return array_view<const uint8_t>(stagingPtr_ ? (stagingPtr_ + vertexDataSize_) : nullptr, indexDataSize_);
}

array_view<const uint8_t> MeshBuilder::GetJointData() const
{
    return array_view<const uint8_t>(
        stagingPtr_ ? (stagingPtr_ + vertexDataSize_ + indexDataSize_) : nullptr, jointDataSize_);
}

array_view<const uint8_t> MeshBuilder::GetMorphTargetData() const
{
    return array_view<const uint8_t>(
        stagingPtr_ ? (stagingPtr_ + vertexDataSize_ + indexDataSize_ + jointDataSize_) : nullptr, targetDataSize_);
}

array_view<const float> MeshBuilder::GetJointBoundsData() const
{
    return array_view(reinterpret_cast<const float*>(jointBoundsData_.data()),
        jointBoundsData_.size() * SIZE_OF_VALUE_TYPE_V<decltype(jointBoundsData_)> / sizeof(float));
}

array_view<const MeshComponent::Submesh> MeshBuilder::GetSubmeshes() const
{
    return array_view<const MeshComponent::Submesh>(submeshes_);
}

uint32_t MeshBuilder::GetVertexCount() const
{
    return vertexCount_;
}

uint32_t MeshBuilder::GetIndexCount() const
{
    return indexCount_;
}

void MeshBuilder::CreateGpuResources(const GpuBufferCreateInfo& createInfo)
{
    GenerateMissingAttributes();

    bufferHandles_ = CreateGpuBuffers(
        renderContext_, vertexDataSize_, indexDataSize_, indexCount_, jointDataSize_, targetDataSize_, createInfo);

    StageToBuffers(renderContext_, vertexDataSize_, indexDataSize_, jointDataSize_, targetDataSize_, bufferHandles_,
        stagingBuffer_);
}

void MeshBuilder::CreateGpuResources()
{
    CreateGpuResources({});
}

Entity MeshBuilder::CreateMesh(IEcs& ecs) const
{
    if (!vertexDataSize_) {
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
    if ((uid == IMeshBuilder::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* MeshBuilder::GetInterface(const Uid& uid)
{
    if ((uid == IMeshBuilder::UID) || (uid == IInterface::UID)) {
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

// Private methods
MeshBuilder::BufferEntities MeshBuilder::CreateBuffers(IEcs& ecs) const
{
    BufferEntities entities;

    BufferHandles handles;
    if (bufferHandles_.vertexBuffer) {
        handles = bufferHandles_;
    } else {
        GenerateMissingAttributes();
        handles = CreateGpuBuffers(
            renderContext_, vertexDataSize_, indexDataSize_, indexCount_, jointDataSize_, targetDataSize_, {});
        StageToBuffers(
            renderContext_, vertexDataSize_, indexDataSize_, jointDataSize_, targetDataSize_, handles, stagingBuffer_);
    }

    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    if (!renderHandleManager) {
        return entities;
    }

    auto& em = ecs.GetEntityManager();

    // Create vertex buffer for this mesh.
    entities.vertexBuffer = CreateBuffer(em, *renderHandleManager, handles.vertexBuffer);

    // Create index buffer for this mesh.
    if (handles.indexBuffer) {
        entities.indexBuffer = CreateBuffer(em, *renderHandleManager, handles.indexBuffer);
    }

    if (handles.jointBuffer) {
        entities.jointBuffer = CreateBuffer(em, *renderHandleManager, handles.jointBuffer);
    }

    if (handles.morphBuffer) {
        entities.morphBuffer = CreateBuffer(em, *renderHandleManager, handles.morphBuffer);
    }
    return entities;
}

void MeshBuilder::GenerateMissingAttributes() const
{
    if (auto buffer = stagingPtr_; buffer) {
        auto submeshIt = submeshes_.begin();
        for (auto& submesh : submeshInfos_) {
            if (!vertexData_.empty() &&
                (!submesh.hasNormals || !submesh.hasUv0 || (submesh.info.tangents && !submesh.hasTangents))) {
                MeshComponent::Submesh& submeshDesc = *submeshIt;
                const DataBuffer indexData { (submesh.info.indexType == IndexType::CORE_INDEX_TYPE_UINT16)
                                                 ? BASE_FORMAT_R16_UINT
                                                 : BASE_FORMAT_R32_UINT,
                    static_cast<uint32_t>((submesh.info.indexType == IndexType::CORE_INDEX_TYPE_UINT16)
                                              ? sizeof(uint16_t)
                                              : sizeof(uint32_t)),
                    { indexData_.data() + submesh.indexOffset, submesh.indexSize } };

                // Reserve space for the to be generated normals and uvs
                vertexData_.reserve(vertexData_.size() +
                                    (submesh.hasNormals ? 0 : submesh.info.vertexCount * sizeof(Math::Vec3)) +
                                    (submesh.hasUv0 ? 0 : submesh.info.vertexCount * sizeof(Math::Vec2)));

                DataBuffer positionData { BASE_FORMAT_R32G32B32_SFLOAT, sizeof(Math::Vec3),
                    { vertexData_.data() + submesh.positionOffset, submesh.positionSize } };

                DataBuffer normalData { BASE_FORMAT_R32G32B32_SFLOAT, sizeof(Math::Vec3),
                    { vertexData_.data() + submesh.normalOffset, submesh.normalSize } };

                if (!submesh.hasNormals) {
                    const auto offset = vertexData_.size();
                    GenerateDefaultNormals(vertexData_, indexData, positionData, submesh.info.vertexCount);
                    submesh.normalOffset = static_cast<int32_t>(offset);
                    submesh.normalSize = static_cast<uint32_t>(vertexData_.size() - offset);
                    normalData.buffer = { vertexData_.data() + submesh.normalOffset, submesh.normalSize };

                    auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_NOR];
                    WriteData(normalData, submesh, MeshComponent::Submesh::DM_VB_NOR, acc.offset, acc.byteSize, buffer);
                    submesh.hasNormals = true;
                }

                DataBuffer uvData { BASE_FORMAT_R32G32_SFLOAT, sizeof(Math::Vec2),
                    { vertexData_.data() + submesh.uvOffset, submesh.uvSize } };
                if (!submesh.hasUv0) {
                    const auto offset = vertexData_.size();
                    GenerateDefaultUvs(vertexData_, submesh.info.vertexCount);
                    submesh.uvOffset = static_cast<int32_t>(offset);
                    submesh.uvSize = static_cast<uint32_t>(vertexData_.size() - offset);
                    uvData.buffer = { vertexData_.data() + submesh.uvOffset, submesh.uvSize };

                    auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_UV0];
                    WriteData(uvData, submesh, MeshComponent::Submesh::DM_VB_UV0, acc.offset, acc.byteSize, buffer);
                    submesh.hasUv0 = true;
                }

                if (submesh.info.tangents && !submesh.hasTangents) {
                    DataBuffer tangentData;
                    vector<uint8_t> generatedTangents;
                    GenerateDefaultTangents(tangentData, generatedTangents, indexData, positionData, normalData, uvData,
                        submesh.info.vertexCount);

                    auto& acc = submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_TAN];
                    if (WriteData(tangentData, submesh, MeshComponent::Submesh::DM_VB_TAN, acc.offset, acc.byteSize,
                            buffer)) {
                        submeshDesc.flags |= MeshComponent::Submesh::FlagBits::TANGENTS_BIT;
                        submesh.hasTangents = true;
                    }
                }
            }
            ++submeshIt;
        }
    }
}

void MeshBuilder::GatherDeltasP(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
    uint32_t targetSize, const MeshBuilder::DataBuffer& targetPositions)
{
    if (targetPositions.stride > (targetPositions.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto posFormat = GetFormatSpec(targetPositions.format);
    if (posFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto posElementSize = posFormat.componentCount * posFormat.componentByteSize;
        posElementSize > targetPositions.stride) {
        return;
    }

    // Target data starts after base
    uint32_t targetOffset = baseOffset + targetSize;

    auto index = reinterpret_cast<uint32_t*>(dst + indexOffset);
    if (posFormat.format == BASE_FORMAT_R32G32B32_SFLOAT) {
        // special case which matches glTF 2.0. morph targets are three float components.
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position, normal and tangent deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                auto pos = *reinterpret_cast<const Math::Vec3*>(
                    targetPositions.buffer.data() + targetPositions.stride * vertexIndex);
                const auto zeroDelta = (pos == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);

                target->pos = Math::Vec4(pos, static_cast<float>(vertex));
                ++target;
            }
            // Store the size and indexOffset of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    } else {
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position, normal and tangent deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                Math::Vec3 pos;
                auto ptr = targetPositions.buffer.data() + targetPositions.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(pos.data), posFormat.componentCount); ++i) {
                    pos[i] = posFormat.toIntermediate(ptr + i * posFormat.componentByteSize);
                }
                const auto zeroDelta = (pos == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);

                target->pos = Math::Vec4(pos, static_cast<float>(vertex));
                ++target;
            }
            // Store the size and indexOffset of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    }
}

void MeshBuilder::GatherDeltasPN(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
    uint32_t targetSize, const MeshBuilder::DataBuffer& targetPositions, const MeshBuilder::DataBuffer& targetNormals)
{
    if (targetPositions.stride > (targetPositions.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto posFormat = GetFormatSpec(targetPositions.format);
    if (posFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto posElementSize = posFormat.componentCount * posFormat.componentByteSize;
        posElementSize > targetPositions.stride) {
        return;
    }

    if (targetNormals.stride > (targetNormals.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto norFormat = GetFormatSpec(targetNormals.format);
    if (norFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto norElementSize = norFormat.componentCount * norFormat.componentByteSize;
        norElementSize > targetNormals.stride) {
        return;
    }

    // Target data starts after base
    uint32_t targetOffset = baseOffset + targetSize;

    auto index = reinterpret_cast<uint32_t*>(dst + indexOffset);
    if (posFormat.format == BASE_FORMAT_R32G32B32_SFLOAT && norFormat.format == BASE_FORMAT_R32G32B32_SFLOAT) {
        // special case which matches glTF 2.0. morph targets are three float components.
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position and normal deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                auto pos = *reinterpret_cast<const Math::Vec3*>(
                    targetPositions.buffer.data() + targetPositions.stride * vertexIndex);
                auto nor = *reinterpret_cast<const Math::Vec3*>(
                    targetNormals.buffer.data() + targetNormals.stride * vertexIndex);
                const auto zeroDelta = (pos == Math::Vec3 {} && nor == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);

                target->pos = Math::Vec4(pos, static_cast<float>(vertex));
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                target->nortan.x = Math::PackHalf2X16({ nor.x, nor.y });
                target->nortan.y = Math::PackHalf2X16({ nor.z, 0.f });
#else
                target->nor = Math::Vec4(nor, 0.f);
#endif
                ++target;
            }
            // Store the size of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    } else {
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position and normal deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                Math::Vec3 pos;
                auto ptr = targetPositions.buffer.data() + targetPositions.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(pos.data), posFormat.componentCount); ++i) {
                    pos[i] = posFormat.toIntermediate(ptr + i * posFormat.componentByteSize);
                }
                Math::Vec3 nor;
                ptr = targetNormals.buffer.data() + targetNormals.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(nor.data), norFormat.componentCount); ++i) {
                    nor[i] = norFormat.toIntermediate(ptr + i * norFormat.componentByteSize);
                }
                const auto zeroDelta = (pos == Math::Vec3 {} && nor == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);

                target->pos = Math::Vec4(pos, static_cast<float>(vertex));
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                target->nortan.x = Math::PackHalf2X16({ nor.data[R], nor.data[G] });
                target->nortan.y = Math::PackHalf2X16({ nor.data[B], 0.f });
#else
                target->nor = Math::Vec4(nor.data[R], nor.data[G], nor.data[B], 0.f);
#endif
                ++target;
            }
            // Store the size of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    }
}

void MeshBuilder::GatherDeltasPT(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
    uint32_t targetSize, const MeshBuilder::DataBuffer& targetPositions, const MeshBuilder::DataBuffer& targetTangents)
{}

void MeshBuilder::GatherDeltasPNT(SubmeshExt& submesh, uint8_t* dst, uint32_t baseOffset, uint32_t indexOffset,
    uint32_t targetSize, const MeshBuilder::DataBuffer& targetPositions, const MeshBuilder::DataBuffer& targetNormals,
    const MeshBuilder::DataBuffer& targetTangents)
{
    if (targetPositions.stride > (targetPositions.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto posFormat = GetFormatSpec(targetPositions.format);
    if (posFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto posElementSize = posFormat.componentCount * posFormat.componentByteSize;
        posElementSize > targetPositions.stride) {
        return;
    }

    if (targetNormals.stride > (targetNormals.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto norFormat = GetFormatSpec(targetNormals.format);
    if (norFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto norElementSize = norFormat.componentCount * norFormat.componentByteSize;
        norElementSize > targetNormals.stride) {
        return;
    }

    if (targetTangents.stride > (targetTangents.buffer.size() / submesh.info.vertexCount)) {
        return;
    }
    const auto tanFormat = GetFormatSpec(targetTangents.format);
    if (tanFormat.format == BASE_FORMAT_UNDEFINED) {
        CORE_LOG_E("position format (%u) not supported", posFormat.format);
        return;
    }
    if (const auto tanElementSize = tanFormat.componentCount * tanFormat.componentByteSize;
        tanElementSize > targetTangents.stride) {
        return;
    }

    // Target data starts after base
    uint32_t targetOffset = baseOffset + targetSize;

    auto index = reinterpret_cast<uint32_t*>(dst + indexOffset);
    if (posFormat.format == BASE_FORMAT_R32G32B32_SFLOAT && norFormat.format == BASE_FORMAT_R32G32B32_SFLOAT &&
        tanFormat.format == BASE_FORMAT_R32G32B32_SFLOAT) {
        // special case which matches glTF 2.0. morph targets are three float components.
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position, normal and tangent deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                auto pos = *reinterpret_cast<const Math::Vec3*>(
                    targetPositions.buffer.data() + targetPositions.stride * vertexIndex);
                auto nor = *reinterpret_cast<const Math::Vec3*>(
                    targetNormals.buffer.data() + targetNormals.stride * vertexIndex);
                auto tan = *reinterpret_cast<const Math::Vec3*>(
                    targetTangents.buffer.data() + targetTangents.stride * vertexIndex);
                const auto zeroDelta = (pos == Math::Vec3 {} && nor == Math::Vec3 {} && tan == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);
                target->pos = Math::Vec4(pos, static_cast<float>(vertex));

#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                target->nortan.x = Math::PackHalf2X16({ nor.x, nor.y });
                target->nortan.y = Math::PackHalf2X16({ nor.z, 0.f });
                target->nortan.z = Math::PackHalf2X16({ tan.x, tan.y });
                target->nortan.w = Math::PackHalf2X16({ tan.z, 0.f });
#else
                target->nor = Math::Vec4(nor, 0.f);
                target->tan = Math::Vec4(tan, 0.f);
#endif
                ++target;
            }
            // Store the size of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    } else {
        for (uint32_t trg = 0; trg < submesh.info.morphTargetCount; trg++) {
            submesh.morphTargets[trg].offset = targetOffset;
            const auto startTarget = reinterpret_cast<MorphInputData*>(dst + targetOffset);
            auto target = startTarget;
            for (uint32_t vertex = 0; vertex < submesh.info.vertexCount; ++vertex) {
                // for each vertex in target check that position, normal and tangent deltas are non-zero.
                const auto vertexIndex = vertex + (trg * submesh.info.vertexCount);
                Math::Vec3 pos;
                auto ptr = targetPositions.buffer.data() + targetPositions.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(pos.data), posFormat.componentCount); ++i) {
                    pos[i] = posFormat.toIntermediate(ptr + i * posFormat.componentByteSize);
                }
                Math::Vec3 nor;
                ptr = targetNormals.buffer.data() + targetNormals.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(nor.data), norFormat.componentCount); ++i) {
                    nor[i] = norFormat.toIntermediate(ptr + i * norFormat.componentByteSize);
                }
                Math::Vec3 tan;
                ptr = targetTangents.buffer.data() + targetTangents.stride * vertexIndex;
                for (auto i = 0U; i < Math::min(countof(tan.data), tanFormat.componentCount); ++i) {
                    tan[i] = tanFormat.toIntermediate(ptr + i * tanFormat.componentByteSize);
                }
                const auto zeroDelta = (pos == Math::Vec3 {} && nor == Math::Vec3 {} && tan == Math::Vec3 {});
                // store offset for each non-zero
                *index++ = zeroDelta ? UINT32_MAX : ((targetOffset - baseOffset) / sizeof(MorphInputData));
                if (zeroDelta) {
                    continue;
                }
                targetOffset += sizeof(MorphInputData);

                target->pos = Math::Vec4(pos, static_cast<float>(vertex));
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
                target->nortan.x = Math::PackHalf2X16({ nor.data[R], nor.data[G] });
                target->nortan.y = Math::PackHalf2X16({ nor.data[B], 0.f });
                target->nortan.z = Math::PackHalf2X16({ tan.data[R], tan.data[G] });
                target->nortan.w = Math::PackHalf2X16({ tan.data[B], 0.f });
#else
                target->nor = Math::Vec4(nor.data[R], nor.data[G], nor.data[B], 0.f);
                target->tan = Math::Vec4(tan.data[R], tan.data[G], tan.data[B], 0.f);
#endif
                ++target;
            }
            // Store the size of the gathered deltas.
            const auto byteSize =
                static_cast<uint32_t>(reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(startTarget));
            submesh.morphTargets[trg].byteSize = byteSize;
        }
    }
}

void MeshBuilder::CalculateJointBounds(
    const DataBuffer& jointData, const DataBuffer& weightData, const DataBuffer& positionData)
{
    // Calculate joint bounds as the bounds of the vertices that the joint references.

    const auto jointFormat = GetFormatSpec(jointData.format);
    if (jointFormat.format == BASE_FORMAT_UNDEFINED) {
        return;
    }
    if (const auto jointElementSize = jointFormat.componentCount * jointFormat.componentByteSize;
        jointElementSize > jointData.stride) {
        return;
    }

    const auto weightFormat = GetFormatSpec(weightData.format);
    if (weightFormat.format == BASE_FORMAT_UNDEFINED) {
        return;
    }
    if (const auto weightElementSize = weightFormat.componentCount * weightFormat.componentByteSize;
        weightElementSize > weightData.stride) {
        return;
    }

    const auto positionFormat = GetFormatSpec(positionData.format);
    if (positionFormat.format == BASE_FORMAT_UNDEFINED) {
        return;
    }
    if (const auto positionElementSize = positionFormat.componentCount * positionFormat.componentByteSize;
        positionElementSize > positionData.stride) {
        return;
    }

    const auto* weights = weightData.buffer.data();
    const auto* joints = jointData.buffer.data();

    const size_t jointIndexCount = jointData.buffer.size() / jointData.stride;

    // Find the amount of referenced joints
    size_t maxJointIndex = 0;
    for (size_t i = 0; i < jointIndexCount; ++i) {
        float fWeights[4U];
        for (auto j = 0U; j < weightFormat.componentCount; ++j) {
            fWeights[j] = weightFormat.toIntermediate(weights + j * weightFormat.componentByteSize);
        }
        weights += weightData.stride;
        for (size_t w = 0; w < countof(fWeights); ++w) {
            // Ignore joints with weight that is effectively 0.0
            if (fWeights[w] >= Math::EPSILON) {
                const uint8_t jointIndex = joints[jointFormat.componentByteSize * w];

                if (jointIndex > maxJointIndex) {
                    maxJointIndex = jointIndex;
                }
            }
        }
        joints += jointData.stride;
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

    weights = weightData.buffer.data();
    joints = jointData.buffer.data();
    const auto* positions = positionData.buffer.data();
    for (auto i = 0U; i < jointIndexCount; ++i) {
        // Each vertex can reference 4 joint indices.
        Math::Vec3 pos;
        auto ptr = positions + i * positionData.stride;
        for (auto j = 0U; j < positionFormat.componentCount; ++j) {
            pos[j] = positionFormat.toIntermediate(ptr + j * positionFormat.componentByteSize);
        }

        float fWeights[4U];
        for (auto j = 0U; j < weightFormat.componentCount; ++j) {
            fWeights[j] = weightFormat.toIntermediate(weights + j * weightFormat.componentByteSize);
        }
        weights += weightData.stride;
        for (size_t w = 0; w < countof(fWeights); ++w) {
            if (fWeights[w] < Math::EPSILON) {
                // Ignore joints with weight that is effectively 0.0
                continue;
            }

            auto& boundsData = jointBoundsData_[joints[w]];
            boundsData.min = Math::min(boundsData.min, pos);
            boundsData.max = Math::max(boundsData.max, pos);
        }
        joints += jointData.stride;
    }
}

bool MeshBuilder::WriteData(const DataBuffer& srcData, const SubmeshExt& submesh, uint32_t attributeLocation,
    uint32_t& byteOffset, uint32_t& byteSize, uint8_t* dst) const
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
            OutputBuffer dstData { attributeDesc->format, bindingDesc->stride, { dst + byteOffset, byteSize } };
            Fill(dstData, srcData, submesh.info.vertexCount);
            return true;
        }
    }
    return false;
}
CORE3D_END_NAMESPACE()
