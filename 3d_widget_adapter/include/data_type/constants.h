/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_CONSTANTS_H
#define OHOS_RENDER_3D_CONSTANTS_H

#include <cstdint>

namespace OHOS::Render3D {
enum class AnimationState : uint8_t {
    PLAY,
    PAUSE,
    STOP,
};

enum class BackgroundType : uint8_t {
    CUBE_MAP,
    TRANSPARENT,
};

enum class SurfaceType : uint8_t {
    UNDEFINE,
    SURFACE_WINDOW, // bufferqueue dss
    SURFACE_TEXTURE, // bufferqueue texture
    SURFACE_BUFFER, // no buffer queue
};

enum class LightType : uint8_t {
    INVALID = 0,
    DIRECTIONAL = 1,
    POINT = 2,
    SPOT = 3
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER3D_CONSTANTS_H
