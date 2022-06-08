/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    RENDER_NS::RenderNodeGraphDesc GetRenderNodeGraphDesc(
        const RenderScene& renderScene, const uint32_t flags) const override;

private:
    void InitRenderNodeGraphs();
    RENDER_NS::RenderNodeGraphDesc SelectBaseDesc(const RenderCamera& renderCamera) const;

    RENDER_NS::IRenderContext& context_;
    const RENDER_NS::DeviceBackendType backendType_ { RENDER_NS::DeviceBackendType::VULKAN };

    RENDER_NS::RenderNodeGraphDesc rngdScene_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrp_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrpMsaa_;
    RENDER_NS::RenderNodeGraphDesc rngdCamLwrpMsaaGles_;
    RENDER_NS::RenderNodeGraphDesc rngdCamHdr_;
    RENDER_NS::RenderNodeGraphDesc rngdCamHdrMsaa_;
    RENDER_NS::RenderNodeGraphDesc rngdReflCam_;
    RENDER_NS::RenderNodeGraphDesc rngdCamPrePass_;

    RENDER_NS::RenderNodeGraphDesc rngdDeferred_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_RENDER_UTIL_H
