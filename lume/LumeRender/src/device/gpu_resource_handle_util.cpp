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

#include "gpu_resource_handle_util.h"

#include <render/namespace.h>
#include <render/resource_handle.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()

// Internal RendererDataHandleUtil API implementation
namespace RenderHandleUtil {
namespace {
#if (RENDER_VALIDATION_ENABLED == 1)
constexpr uint64_t HANDLE_ID_SIZE { RES_HANDLE_ID_MASK >> RES_HANDLE_ID_SHIFT };
#endif
} // namespace

RenderHandle CreateGpuResourceHandle(const RenderHandleType type, const RenderHandleInfoFlags infoFlags,
    const uint32_t index, const uint32_t generationIndex)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (index >= HANDLE_ID_SIZE) {
        PLUGIN_LOG_E("index (%u), exceeds max index (%u)", index, static_cast<uint32_t>(HANDLE_ID_SIZE));
    }
#endif

    return { (((uint64_t)infoFlags << RES_HANDLE_ADDITIONAL_INFO_SHIFT) & RES_HANDLE_ADDITIONAL_INFO_MASK) |
             (((uint64_t)generationIndex << RES_HANDLE_GENERATION_SHIFT) & RES_HANDLE_GENERATION_MASK) |
             ((uint64_t)((index << RES_HANDLE_ID_SHIFT) & RES_HANDLE_ID_MASK)) |
             ((uint64_t)(type)&RENDER_HANDLE_TYPE_MASK) };
}

RenderHandle CreateGpuResourceHandle(const RenderHandleType type, const RenderHandleInfoFlags infoFlags,
    const uint32_t index, const uint32_t generationIndex, const uint32_t hasNameId)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (index >= HANDLE_ID_SIZE) {
        PLUGIN_LOG_E("index (%u), exceeds max index (%u)", index, static_cast<uint32_t>(HANDLE_ID_SIZE));
    }
#endif
    return { (((uint64_t)hasNameId << RES_HANDLE_HAS_NAME_SHIFT) & RES_HANDLE_HAS_NAME_MASK) |
             (((uint64_t)infoFlags << RES_HANDLE_ADDITIONAL_INFO_SHIFT) & RES_HANDLE_ADDITIONAL_INFO_MASK) |
             (((uint64_t)generationIndex << RES_HANDLE_GENERATION_SHIFT) & RES_HANDLE_GENERATION_MASK) |
             ((uint64_t)((index << RES_HANDLE_ID_SHIFT) & RES_HANDLE_ID_MASK)) |
             ((uint64_t)(type)&RENDER_HANDLE_TYPE_MASK) };
}

RenderHandle CreateHandle(const RenderHandleType type, const uint32_t index)
{
    return CreateHandle(type, index, 0);
}

RenderHandle CreateHandle(const RenderHandleType type, const uint32_t index, const uint32_t generationIndex)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (index >= HANDLE_ID_SIZE) {
        PLUGIN_LOG_E("index (%u), exceeds max index (%u)", index, static_cast<uint32_t>(HANDLE_ID_SIZE));
    }
#endif
    return { (((uint64_t)generationIndex << RES_HANDLE_GENERATION_SHIFT) & RES_HANDLE_GENERATION_MASK) |
             ((uint64_t)((index << RES_HANDLE_ID_SHIFT) & RES_HANDLE_ID_MASK)) |
             ((uint64_t)(type)&RENDER_HANDLE_TYPE_MASK) };
}

RenderHandle CreateHandle(
    const RenderHandleType type, const uint32_t index, const uint32_t generationIndex, const uint32_t additionalData)
{
    RenderHandle handle = CreateHandle(type, index, generationIndex);
    handle.id |= (((uint64_t)additionalData << RES_HANDLE_ADDITIONAL_INFO_SHIFT) & RES_HANDLE_ADDITIONAL_INFO_MASK);
    return handle;
}

EngineResourceHandle CreateEngineResourceHandle(
    const RenderHandleType type, const uint32_t index, const uint32_t generationIndex)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (index >= HANDLE_ID_SIZE) {
        PLUGIN_LOG_E("index (%u), exceeds max index (%u)", index, static_cast<uint32_t>(HANDLE_ID_SIZE));
    }
#endif
    return { (((uint64_t)generationIndex << RES_HANDLE_GENERATION_SHIFT) & RES_HANDLE_GENERATION_MASK) |
             ((uint64_t)((index << RES_HANDLE_ID_SHIFT) & RES_HANDLE_ID_MASK)) |
             ((uint64_t)(type)&RENDER_HANDLE_TYPE_MASK) };
}
} // namespace RenderHandleUtil
RENDER_END_NAMESPACE()
