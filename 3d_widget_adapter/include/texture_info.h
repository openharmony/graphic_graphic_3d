/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_TEXTURE_INFO_H
#define OHOS_RENDER_3D_TEXTURE_INFO_H

#include <GLES/gl.h>
#include <cstdint>

namespace OHOS::Render3D {
struct TextureInfo {
    uint32_t width_ = 0U;
    uint32_t height_ = 0U;
    GLuint textureId_ = 0U;
    void* nativeWindow_ = nullptr;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_TEXTURE_INFO_H
