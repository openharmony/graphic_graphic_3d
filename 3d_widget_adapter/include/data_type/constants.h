/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_CONSTANTS_H
#define OHOS_RENDER_3D_CONSTANTS_H

#include <cstdint>

namespace OHOS::Render3D {
enum class AnimationState {
    PLAY,
    PAUSE,
    STOP,
};

enum class SceneViewerBackgroundType {
    CUBE_MAP,
    TRANSPARENT,
};

} // namespace OHOS::Render3D
#endif // OHOS_RENDER3D_CONSTANTS_H
