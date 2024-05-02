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

#ifndef API_RENDER_UTIL_IRENDER_UTIL_H
#define API_RENDER_UTIL_IRENDER_UTIL_H

#include <cstdint>

#include <base/containers/string.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_util_irenderutil */
class IRenderFrameUtil;

/**
 * Render timings
 */
struct RenderTimings {
    struct Times {
        /** Time stamp at the beginning of RenderFrame() */
        int64_t begin { 0 };
        /** Time stamp at the end of RenderFrame() */
        int64_t end { 0 };

        /** Time stamp at the beginning of backend command list processing */
        int64_t beginBackend { 0 };
        /** Time stamp at the end of backend command list processing and submits */
        int64_t endBackend { 0 };
        /** Time stamp at the beginning of backend presentation start */
        int64_t beginBackendPresent { 0 };
        /** Time stamp at the end of backend presentation */
        int64_t endBackendPresent { 0 };
    };
    /** Current results after RenderFrame() has returned */
    Times frame;
    /** Previous frame results after RenderFrame() has returned */
    Times prevFrame;
};

/** Render handle description */
struct RenderHandleDesc {
    /** Type */
    RenderHandleType type { RenderHandleType::UNDEFINED };
    /** Additional ID */
    uint64_t id { 0 };
    /** Reference count for this handle as seen from the client side */
    int32_t refCount { 0 };
    /** Name and/or path of the resource */
    BASE_NS::string name;
    /** Additional name of the resource */
    BASE_NS::string additionalName;
};

/** Interface for rendering utilities.
 */
class IRenderUtil {
public:
    /** Get description for given handle.
     * @param handle Render handle reference of the resource.
     * @return RenderHandleDesc Return render handle desc for given handle.
     */
    virtual RenderHandleDesc GetRenderHandleDesc(const RenderHandleReference& handle) const = 0;

    /** Get handle for given description.
     * @param desc Render handle description.
     * @return RenderHandleReference Return render handle for given desc.
     */
    virtual RenderHandleReference GetRenderHandle(const RenderHandleDesc& desc) const = 0;

    /** Get last render timings. Should be usually called after RenderFrame() has returned.
     * @return RenderTimings Results from the last RenderFrame() call.
     */
    virtual RenderTimings GetRenderTimings() const = 0;

    /** Get render frame util with frame related utilities.
     * @return Reference to render frame util interface.
     */
    virtual IRenderFrameUtil& GetRenderFrameUtil() const = 0;

protected:
    IRenderUtil() = default;
    virtual ~IRenderUtil() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_UTIL_IRENDER_UTIL_H
