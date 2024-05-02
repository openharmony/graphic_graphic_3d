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

#ifndef DEFAULT_ENGINE_CONSTANTS_H
#define DEFAULT_ENGINE_CONSTANTS_H

#include <base/containers/string_view.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_engine_defaultengineconstants
 *  @{
 */
/** Default GPU resource constants */
struct DefaultEngineGpuResourceConstants {
    /** Default built-in backbuffer/swapchain */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_BACKBUFFER { "CORE_DEFAULT_BACKBUFFER" };
    /** Default backbuffer/swapchain depth (when rendering scene directly to backbuffer) */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_BACKBUFFER_DEPTH { "CORE_DEFAULT_BACKBUFFER_DEPTH" };

    /** Default swapchain prefix */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SWAPCHAIN { "CORE_DEFAULT_SWAPCHAIN_" };

    /** Default GPU image, black */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_GPU_IMAGE { "CORE_DEFAULT_GPU_IMAGE" };
    /** Default GPU image, white */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_GPU_IMAGE_WHITE { "CORE_DEFAULT_GPU_IMAGE_WHITE" };

    /** Default GPU buffer, 1024 bytes */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_GPU_BUFFER { "CORE_DEFAULT_GPU_BUFFER" };

    /** Default sampler, nearest repeat */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_NEAREST_REPEAT {
        "CORE_DEFAULT_SAMPLER_NEAREST_REPEAT"
    };
    /** Default sampler, nearest clamp */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_NEAREST_CLAMP {
        "CORE_DEFAULT_SAMPLER_NEAREST_CLAMP"
    };
    /** Default sampler, linear repeat */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_LINEAR_REPEAT {
        "CORE_DEFAULT_SAMPLER_LINEAR_REPEAT"
    };
    /** Default sampler, linear clamp */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_LINEAR_CLAMP {
        "CORE_DEFAULT_SAMPLER_LINEAR_CLAMP"
    };
    /** Default sampler, linear mipmap repeat */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT {
        "CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT"
    };
    /** Default sampler, linear mipmap clamp */
    static constexpr const BASE_NS::string_view CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP {
        "CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP"
    };
};
/** @} */
RENDER_END_NAMESPACE()

#endif // DEFAULT_ENGINE_CONSTANTS_H
