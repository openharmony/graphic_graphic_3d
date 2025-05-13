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

#ifndef SCENE_INTERFACE_RESOURCE_BITMAP_INFO_H
#define SCENE_INTERFACE_RESOURCE_BITMAP_INFO_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>

#include <base/util/formats.h>
#include <core/resources/intf_resource.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

enum class ImageLoadFlags : uint32_t {
    /** Generate mipmaps if not already included in the image file. */
    GENERATE_MIPS = 0x00000001,
    /** Force linear RGB. */
    FORCE_LINEAR_RGB_BIT = 0x00000002,
    /** Force SRGB. */
    FORCE_SRGB_BIT = 0x00000004,
    /** Force grayscale. */
    FORCE_GRAYSCALE_BIT = 0x00000008,
    /** Flip image vertically when loaded. */
    FLIP_VERTICALLY_BIT = 0x00000010,
    /** Premultiply color values with the alpha value if an alpha channel is present. */
    PREMULTIPLY_ALPHA = 0x00000020,
};
constexpr inline ImageLoadFlags operator|(ImageLoadFlags l, ImageLoadFlags r)
{
    return ImageLoadFlags(uint32_t(l) | uint32_t(r));
}

/** Image usage flags */
enum class ImageUsageFlag : uint32_t {
    /** Transfer source bit */
    TRANSFER_SRC_BIT = 0x00000001,
    /** Transfer destination bit */
    TRANSFER_DST_BIT = 0x00000002,
    /** Sampled bit */
    SAMPLED_BIT = 0x00000004,
    /** Storage bit */
    STORAGE_BIT = 0x00000008,
    /** Color attachment bit */
    COLOR_ATTACHMENT_BIT = 0x00000010,
    /** Depth stencil attachment bit */
    DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    /** Transient attachment bit */
    TRANSIENT_ATTACHMENT_BIT = 0x00000040,
    /** Input attachment bit */
    INPUT_ATTACHMENT_BIT = 0x00000080,
    /** Fragment shading rate attachment bit */
    FRAGMENT_SHADING_RATE_ATTACHMENT_BIT = 0x00000100,
};
constexpr inline ImageUsageFlag operator|(ImageUsageFlag l, ImageUsageFlag r)
{
    return ImageUsageFlag(uint32_t(l) | uint32_t(r));
}

enum class MemoryPropertyFlag : uint32_t {
    /** Device local bit */
    DEVICE_LOCAL_BIT = 0x00000001,
    /** Host visible bit */
    HOST_VISIBLE_BIT = 0x00000002,
    /** Host visible bit */
    HOST_COHERENT_BIT = 0x00000004,
    /** Host cached bit, always preferred not required for allocation */
    HOST_CACHED_BIT = 0x00000008,
    /** Lazily allocated bit, always preferred not required for allocation */
    LAZILY_ALLOCATED_BIT = 0x00000010,
    /** Protected bit, always preferred not required for allocation */
    PROTECTED_BIT = 0x00000020,
};
constexpr inline MemoryPropertyFlag operator|(MemoryPropertyFlag l, MemoryPropertyFlag r)
{
    return MemoryPropertyFlag(uint32_t(l) | uint32_t(r));
}

/** Engine image creation flags */
enum class EngineImageCreationFlag : uint32_t {
    /** Dynamic barriers */
    DYNAMIC_BARRIERS = 0x00000001,
    /** Reset state on frame borders */
    RESET_STATE_ON_FRAME_BORDERS = 0x00000002,
    /** Generate mips */
    GENERATE_MIPS = 0x00000004,
    /** Scale image when created from data */
    SCALE = 0x00000008,
    /** Destroy is deferred to the end of the current frame */
    DEFERRED_DESTROY = 0x00000010,
};
constexpr inline EngineImageCreationFlag operator|(EngineImageCreationFlag l, EngineImageCreationFlag r)
{
    return EngineImageCreationFlag(uint32_t(l) | uint32_t(r));
}

struct ImageInfo {
    ImageUsageFlag usageFlags {};
    MemoryPropertyFlag memoryFlags {};
    EngineImageCreationFlag creationFlags {};
};

struct ImageLoadInfo {
    ImageLoadFlags loadFlags {};
    ImageInfo info;
};

struct ImageCreateInfo {
    BASE_NS::Math::UVec2 size {};
    BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB };
    ImageInfo info;
};

constexpr ImageCreateInfo DEFAULT_RENDER_TARGET_CREATE_INFO = { {}, BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB,
    { ImageUsageFlag::COLOR_ATTACHMENT_BIT, MemoryPropertyFlag::DEVICE_LOCAL_BIT,
        EngineImageCreationFlag::RESET_STATE_ON_FRAME_BORDERS | EngineImageCreationFlag::DYNAMIC_BARRIERS } };

constexpr ImageCreateInfo DEFAULT_IMAGE_CREATE_INFO = DEFAULT_RENDER_TARGET_CREATE_INFO;

constexpr ImageLoadInfo DEFAULT_IMAGE_LOAD_INFO = { ImageLoadFlags::GENERATE_MIPS,
    ImageInfo { ImageUsageFlag::SAMPLED_BIT | ImageUsageFlag::TRANSFER_DST_BIT | ImageUsageFlag::TRANSFER_SRC_BIT,
        MemoryPropertyFlag::DEVICE_LOCAL_BIT, EngineImageCreationFlag::GENERATE_MIPS } };

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ImageLoadFlags)
META_TYPE(SCENE_NS::ImageInfo)
META_TYPE(SCENE_NS::ImageLoadInfo)
META_TYPE(SCENE_NS::ImageCreateInfo)

#endif
