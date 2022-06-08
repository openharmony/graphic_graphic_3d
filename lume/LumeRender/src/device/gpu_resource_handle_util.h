/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef DEVICE_GPU_RESOURCE_HANDLE_UTIL_H
#define DEVICE_GPU_RESOURCE_HANDLE_UTIL_H

#include <cstdint>

#include <base/namespace.h>
#include <base/util/compile_time_hashes.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
constexpr const uint64_t GPU_RESOURCE_HANDLE_ID_MASK { INVALID_RESOURCE_HANDLE };

struct EngineResourceHandle {
    uint64_t id { GPU_RESOURCE_HANDLE_ID_MASK };
};

inline bool operator==(EngineResourceHandle const& lhs, EngineResourceHandle const& rhs)
{
    return (lhs.id & GPU_RESOURCE_HANDLE_ID_MASK) == (rhs.id & GPU_RESOURCE_HANDLE_ID_MASK);
}

// NOTE: RenderHandleType valid max is currently 15

enum RenderHandleInfoFlagBits {
    CORE_RESOURCE_HANDLE_DYNAMIC_TRACK = 0x00000001,
    CORE_RESOURCE_HANDLE_RESET_ON_FRAME_BORDERS = 0x00000002,
    CORE_RESOURCE_HANDLE_DEPTH_IMAGE = 0x00000004,
    CORE_RESOURCE_HANDLE_IMMEDIATELY_CREATED = 0x00000008,
    CORE_RESOURCE_HANDLE_DEFERRED_DESTROY = 0x00000010,
    CORE_RESOURCE_HANDLE_MAP_OUTSIDE_RENDERER = 0x00000020,
    CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION = 0x00000040, // e.g. hwBuffer ycbcr conversion / oes
    CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE = 0x00000080,
};
using RenderHandleInfoFlags = uint32_t;

namespace RenderHandleUtil {
static constexpr uint64_t RES_ID_MASK { GPU_RESOURCE_HANDLE_ID_MASK };
static constexpr uint64_t RES_HANDLE_ID_MASK { 0x00FFfff0 };
static constexpr uint64_t RES_HANDLE_GENERATION_MASK { 0xFF000000 };
static constexpr uint64_t RES_HANDLE_HAS_NAME_MASK { 0x1000000000000 };
static constexpr uint64_t RES_HANDLE_ADDITIONAL_INFO_MASK { 0xffff00000000 };

static constexpr uint64_t RES_HANDLE_ID_SHIFT { 4 };
static constexpr uint64_t RES_HANDLE_TYPE_SHIFT { 0 };
static constexpr uint64_t RES_HANDLE_GENERATION_SHIFT { 24 };
static constexpr uint64_t RES_HANDLE_HAS_NAME_SHIFT { 48 };
static constexpr uint64_t RES_HANDLE_ADDITIONAL_INFO_SHIFT { 32 };

RenderHandle CreateGpuResourceHandle(const RenderHandleType type, const RenderHandleInfoFlags infoFlags,
    const uint32_t index, const uint32_t generationIndex);
RenderHandle CreateGpuResourceHandle(const RenderHandleType type, const RenderHandleInfoFlags infoFlags,
    const uint32_t index, const uint32_t generationIndex, const uint32_t hasNameId);

// only related to GPU resources
inline bool IsDynamicResource(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_DYNAMIC_TRACK;
}
inline bool IsResetOnFrameBorders(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_RESET_ON_FRAME_BORDERS;
}
inline bool IsDepthImage(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_DEPTH_IMAGE;
}
inline bool IsDeferredDestroy(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_DEFERRED_DESTROY;
}
inline bool IsMappableOutsideRenderer(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_MAP_OUTSIDE_RENDERER;
}
inline bool IsImmediatelyCreated(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_IMMEDIATELY_CREATED;
}
inline bool IsPlatformConversionResource(const RenderHandle handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION;
}
inline uint32_t GetHasNamePart(const RenderHandle handle)
{
    return (handle.id & RES_HANDLE_HAS_NAME_MASK) >> RES_HANDLE_HAS_NAME_SHIFT;
}
inline bool IsGpuBuffer(const RenderHandle& handle)
{
    return RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_BUFFER;
}
inline bool IsGpuImage(const RenderHandle& handle)
{
    return RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_IMAGE;
}
inline bool IsGpuSampler(const RenderHandle& handle)
{
    return RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_SAMPLER;
}
inline bool IsGpuAccelerationStructure(const RenderHandle& handle)
{
    return ((handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT) &
           RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE;
}

RenderHandle CreateHandle(const RenderHandleType type, const uint32_t index);
RenderHandle CreateHandle(const RenderHandleType type, const uint32_t index, const uint32_t generationIndex);
RenderHandle CreateHandle(
    const RenderHandleType type, const uint32_t index, const uint32_t generationIndex, const uint32_t additionalData);

inline uint32_t GetIndexPart(const RenderHandle handle)
{
    return (handle.id & RES_HANDLE_ID_MASK) >> RES_HANDLE_ID_SHIFT;
}
inline uint32_t GetIndexPart(const uint64_t handleId)
{
    return (handleId & RES_HANDLE_ID_MASK) >> RES_HANDLE_ID_SHIFT;
}
inline uint32_t GetGenerationIndexPart(const RenderHandle handle)
{
    return (handle.id & RES_HANDLE_GENERATION_MASK) >> RES_HANDLE_GENERATION_SHIFT;
}
inline uint32_t GetGenerationIndexPart(const uint64_t handleId)
{
    return (handleId & RES_HANDLE_GENERATION_MASK) >> RES_HANDLE_GENERATION_SHIFT;
}

inline uint32_t GetAdditionalData(const RenderHandle handle)
{
    return (handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT;
}
// 0xF (1 << descriptorSetIndex)
inline uint32_t GetPipelineLayoutDescriptorSetMask(const RenderHandle handle)
{
    return (handle.id & RES_HANDLE_ADDITIONAL_INFO_MASK) >> RES_HANDLE_ADDITIONAL_INFO_SHIFT;
}

inline bool IsValid(const EngineResourceHandle& handle)
{
    return handle.id != INVALID_RESOURCE_HANDLE;
}
inline RenderHandleType GetHandleType(const EngineResourceHandle& handle)
{
    return GetHandleType(RenderHandle { handle.id });
}
inline uint32_t GetIndexPart(const EngineResourceHandle& handle)
{
    return GetIndexPart(RenderHandle { handle.id });
}
inline uint32_t GetGenerationIndexPart(const EngineResourceHandle& handle)
{
    return GetGenerationIndexPart(RenderHandle { handle.id });
}
EngineResourceHandle CreateEngineResourceHandle(
    const RenderHandleType type, const uint32_t index, const uint32_t generationIndex);
} // namespace RenderHandleUtil
RENDER_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()
template<typename T>
uint64_t hash(const T& b);

template<>
inline uint64_t hash(const RENDER_NS::RenderHandle& handle)
{
    return handle.id;
}
BASE_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_HANDLE_UTIL_H
