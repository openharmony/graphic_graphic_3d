/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
