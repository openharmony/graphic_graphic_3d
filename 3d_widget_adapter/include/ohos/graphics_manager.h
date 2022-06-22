/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_GRAPHICS_MANAGER_H
#define OHOS_RENDER_3D_GRAPHICS_MANAGER_H

#include "graphics_manager_common.h"
#include <GLES/gl.h>
#include <EGL/egl.h>

namespace OHOS::Render3D {
class GraphicsManager : public GraphicsManagerCommon {
public:
    static GraphicsManager& GetInstance();
    PlatformData GetPlatformData() const override;
private:
    GraphicsManager(const GraphicsManager&) = delete;
    GraphicsManager& operator=(const GraphicsManager&) = delete;
    GraphicsManager() = default;
    virtual ~GraphicsManager();
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHICS_MANAGER_H
