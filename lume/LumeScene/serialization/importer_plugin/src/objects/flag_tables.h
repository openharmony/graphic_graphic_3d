/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_OBJECTS_FLAG_TABLES_H
#define SCENE_IMP_SRC_OBJECTS_FLAG_TABLES_H

#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/resource/image_info.h>

#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {

// clang-format off
static constexpr NamedValue<SCENE_NS::ImageLoadFlags> IMAGE_LOAD_FLAGS_TABLE[] = {
    { "generateMips",      SCENE_NS::ImageLoadFlags::GENERATE_MIPS },
    { "forceLinearRgb",    SCENE_NS::ImageLoadFlags::FORCE_LINEAR_RGB_BIT },
    { "forceSrgb",         SCENE_NS::ImageLoadFlags::FORCE_SRGB_BIT },
    { "forceGrayscale",    SCENE_NS::ImageLoadFlags::FORCE_GRAYSCALE_BIT },
    { "flipVertically",    SCENE_NS::ImageLoadFlags::FLIP_VERTICALLY_BIT },
    { "premultiplyAlpha",  SCENE_NS::ImageLoadFlags::PREMULTIPLY_ALPHA },
};

static constexpr NamedValue<SCENE_NS::ImageUsageFlag> IMAGE_USAGE_FLAGS_TABLE[] = {
    { "transferSource",                 SCENE_NS::ImageUsageFlag::TRANSFER_SRC_BIT },
    { "transferDestination",            SCENE_NS::ImageUsageFlag::TRANSFER_DST_BIT },
    { "sampled",                        SCENE_NS::ImageUsageFlag::SAMPLED_BIT },
    { "storage",                        SCENE_NS::ImageUsageFlag::STORAGE_BIT },
    { "colorAttachment",                SCENE_NS::ImageUsageFlag::COLOR_ATTACHMENT_BIT },
    { "depthStencilAttachment",         SCENE_NS::ImageUsageFlag::DEPTH_STENCIL_ATTACHMENT_BIT },
    { "transientAttachment",            SCENE_NS::ImageUsageFlag::TRANSIENT_ATTACHMENT_BIT },
    { "inputAttachment",                SCENE_NS::ImageUsageFlag::INPUT_ATTACHMENT_BIT },
    { "fragmentShadingRateAttachment",  SCENE_NS::ImageUsageFlag::FRAGMENT_SHADING_RATE_ATTACHMENT_BIT },
};

static constexpr NamedValue<SCENE_NS::MemoryPropertyFlag> MEMORY_FLAGS_TABLE[] = {
    { "deviceLocal",      SCENE_NS::MemoryPropertyFlag::DEVICE_LOCAL_BIT },
    { "hostVisible",      SCENE_NS::MemoryPropertyFlag::HOST_VISIBLE_BIT },
    { "hostCoherent",     SCENE_NS::MemoryPropertyFlag::HOST_COHERENT_BIT },
    { "hostCached",       SCENE_NS::MemoryPropertyFlag::HOST_CACHED_BIT },
    { "lazilyAllocated",  SCENE_NS::MemoryPropertyFlag::LAZILY_ALLOCATED_BIT },
    { "protected",        SCENE_NS::MemoryPropertyFlag::PROTECTED_BIT },
};

static constexpr NamedValue<SCENE_NS::EngineImageCreationFlag> CREATION_FLAGS_TABLE[] = {
    { "dynamicBarriers",           SCENE_NS::EngineImageCreationFlag::DYNAMIC_BARRIERS },
    { "resetStateOnFrameBorders",  SCENE_NS::EngineImageCreationFlag::RESET_STATE_ON_FRAME_BORDERS },
    { "generateMips",              SCENE_NS::EngineImageCreationFlag::GENERATE_MIPS },
    { "scale",                     SCENE_NS::EngineImageCreationFlag::SCALE },
    { "deferredDestroy",           SCENE_NS::EngineImageCreationFlag::DEFERRED_DESTROY },
};

static constexpr NamedValue<SCENE_NS::LightingFlags> LIGHTING_FLAGS_TABLE[] = {
    { "shadowReceiver",         SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT },
    { "shadowCaster",           SCENE_NS::LightingFlags::SHADOW_CASTER_BIT },
    { "punctualLightReceiver",  SCENE_NS::LightingFlags::PUNCTUAL_LIGHT_RECEIVER_BIT },
    { "indirectLightReceiver",  SCENE_NS::LightingFlags::INDIRECT_LIGHT_RECEIVER_BIT },
};

static constexpr NamedValue<SCENE_NS::SamplerFilter> SAMPLER_FILTER_TABLE[] = {
    { "nearest", SCENE_NS::SamplerFilter::NEAREST },
    { "linear",  SCENE_NS::SamplerFilter::LINEAR },
};

static constexpr NamedValue<SCENE_NS::SamplerAddressMode> SAMPLER_ADDRESS_MODE_TABLE[] = {
    { "repeat",           SCENE_NS::SamplerAddressMode::REPEAT },
    { "mirroredRepeat",   SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT },
    { "clampToEdge",      SCENE_NS::SamplerAddressMode::CLAMP_TO_EDGE },
    { "clampToBorder",    SCENE_NS::SamplerAddressMode::CLAMP_TO_BORDER },
    { "mirrorClampToEdge", SCENE_NS::SamplerAddressMode::MIRROR_CLAMP_TO_EDGE },
};
// clang-format on

}  // namespace

SCENE_IMP_END_NAMESPACE()

#endif
