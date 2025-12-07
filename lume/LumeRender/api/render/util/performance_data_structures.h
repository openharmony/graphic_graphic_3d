/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_UTIL_PERFORMANCE_DATA_STRUCTURES_H
#define API_RENDER_UTIL_PERFORMANCE_DATA_STRUCTURES_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct RenderPerformanceDataConstants {
    /** Render Node counters */
    static constexpr const BASE_NS::string_view RENDER_NODE_COUNTERS_NAME { "RenderNode" };

    /** Triangle */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_TRIANGLE { "Backend_Count_Triangle" };
    /** InstanceCount */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_INSTANCECOUNT { "Backend_Count_InstanceCount" };
    /** Draw */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_DRAW { "Backend_Count_Draw" };
    /** DrawIndirect */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_DRAWINDIRECT { "Backend_Count_DrawIndirect" };
    /** Dispatch */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_DISPATCH { "Backend_Count_Dispatch" };
    /** DispatchIndirect */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_DISPATCHINDIRECT { "Backend_Count_DispatchIndirect" };
    /** BindPipeline */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_BINDPIPELINE { "Backend_Count_BindPipeline" };
    /** RenderPass */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_RENDERPASS { "Backend_Count_RenderPass" };
    /** UpdateDescriptorSet */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_UPDATEDESCRIPTORSET {
        "Backend_Count_UpdateDescriptorSet"
    };
    /** BindDescriptorSet */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_BINDDESCRIPTORSET { "Backend_Count_BindDescriptorSet" };

    /** Render counter list */
    static constexpr const BASE_NS::string_view BACKEND_COUNT_LIST[] {
        BACKEND_COUNT_TRIANGLE,
        BACKEND_COUNT_INSTANCECOUNT,
        BACKEND_COUNT_DRAW,
        BACKEND_COUNT_DRAWINDIRECT,
        BACKEND_COUNT_DISPATCH,
        BACKEND_COUNT_DISPATCHINDIRECT,
        BACKEND_COUNT_BINDPIPELINE,
        BACKEND_COUNT_RENDERPASS,
        BACKEND_COUNT_UPDATEDESCRIPTORSET,
        BACKEND_COUNT_BINDDESCRIPTORSET,
    };

    static constexpr BASE_NS::pair<BASE_NS::string_view, int64_t> BACKEND_COUNT_STRING_LIST[] {
        { BACKEND_COUNT_TRIANGLE, 0 },
        { BACKEND_COUNT_INSTANCECOUNT, 0 },
        { BACKEND_COUNT_DRAW, 0 },
        { BACKEND_COUNT_DRAWINDIRECT, 0 },
        { BACKEND_COUNT_DISPATCH, 0 },
        { BACKEND_COUNT_DISPATCHINDIRECT, 0 },
        { BACKEND_COUNT_BINDPIPELINE, 0 },
        { BACKEND_COUNT_RENDERPASS, 0 },
        { BACKEND_COUNT_UPDATEDESCRIPTORSET, 0 },
        { BACKEND_COUNT_BINDDESCRIPTORSET, 0 },
    };
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_UTIL_PERFORMANCE_DATA_STRUCTURES_H
