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

#ifndef CORE_UTIL_RENDER_UTIL_H
#define CORE_UTIL_RENDER_UTIL_H

#include <3d/intf_graphics_context.h>
#include <3d/util/intf_render_util.h>
#include <core/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IGraphicsContext;
class IEngine;
struct RenderCamera;
struct RenderScene;

class RenderUtil : public IRenderUtil {
public:
    explicit RenderUtil(IGraphicsContext& graphicsContext);
    ~RenderUtil() override = default;

    RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const override;
    CameraRenderNodeGraphDescs GetRenderNodeGraphDescs(
        const RenderScene& renderScene, const RenderCamera& renderCamera, const uint32_t flags) const override;
    CameraRenderNodeGraphDescs GetRenderNodeGraphDescs(const RenderScene& renderScene, const RenderCamera& renderCamera,
        const uint32_t flags, const BASE_NS::array_view<const RenderCamera> multiviewCameras) const override;

    RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const uint32_t flags) const override;
    RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const BASE_NS::string& rngFile, const uint32_t flags) const override;

private:
    void InitRenderNodeGraphs();
    RENDER_NS::RenderNodeGraphDesc SelectBaseDesc(const RenderCamera& renderCamera) const;
    RENDER_NS::RenderNodeGraphDesc GetBasePostProcessDesc(const RenderCamera& renderCamera) const;

    RENDER_NS::IRenderContext& context_;
    const RENDER_NS::DeviceBackendType backendType_ { RENDER_NS::DeviceBackendType::VULKAN };

    RENDER_NS::RenderNodeGraphDesc rngdScene_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrp_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrpMsaa_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrpMsaaDepth_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrpMsaaGles_;
    RENDER_NS::RenderNodeGraphDesc rngdCamHdr_;
    RENDER_NS::RenderNodeGraphDesc rngdCamHdrMsaa_;
    RENDER_NS::RenderNodeGraphDesc rngdCamHdrMsaaDepth_;
    RENDER_NS::RenderNodeGraphDesc rngdReflCam_;
    RENDER_NS::RenderNodeGraphDesc rngdReflCamMsaa_;
    RENDER_NS::RenderNodeGraphDesc rngdCamPrePass_;

    RENDER_NS::RenderNodeGraphDesc rngdDeferred_;

    RENDER_NS::RenderNodeGraphDesc rngdPostProcess_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_RENDER_UTIL_H
