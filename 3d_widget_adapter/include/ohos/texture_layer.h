/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef OHOS_RENDER_3D_TEXTURE_LAYER_H
#define OHOS_RENDER_3D_TEXTURE_LAYER_H

#include <cstdint>
#include <future>
#include <memory>
#include <shared_mutex>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <surface_buffer.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkDrawable.h"
#include "include/core/SkImage.h"

#include "texture_info.h"

struct OH_NativeBuffer;
struct NativeWindowBuffer;

namespace OHOS::Render3D {

struct TextureImage {
    explicit TextureImage(const TextureInfo& textureInfo) : textureInfo_(textureInfo) {}
    TextureImage() = default;
    TextureInfo textureInfo_;
    std::function<void()> task_ = nullptr;
    std::shared_future<void> ftr_;
    std::promise<void> pms_;
    sk_sp<SkImage> skImage_;
};

class TextureLayer : public SkDrawable {
public:
    TextureLayer() = default;
    virtual ~TextureLayer();
    TextureInfo CreateRenderTarget(uint32_t width, uint32_t height);
    void SetTextureInfo(const TextureInfo &info);
    void DestroyRenderTarget();
    virtual SkRect onGetBounds() override;
    void OnDraw(SkCanvas* canvas);
    virtual void onDraw(SkCanvas* canvas) override;
    void UpdateRenderFinishFuture(std::shared_future<void> &ftr);
    void SetOffset(int32_t x, int32_t y);
    void SetWH(uint32_t w, uint32_t h);

private:
    void DrawTexture(SkCanvas* canvas);
#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
    void FreeNativeBuffer();
    void FreeNativeWindowBuffer();
    void DrawTextureUnifyRender(SkCanvas* canvas);
#endif
    void AllocGLTexture(uint32_t width, uint32_t height);
    void AllocEglImage(uint32_t width, uint32_t height);

#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
    auto MakePixelImage();
    void ReadPixel();
    void CreateReadFbo();

    GLuint fbo_ = 0u;
    uint8_t *data_ = nullptr;
#endif

    TextureImage image_;
    int32_t offsetX_ = 0u;
    int32_t offsetY_ = 0u;
    uint32_t width_ = 0u;
    uint32_t height_ = 0u;
    std::mutex ftrMut_;
    std::mutex skImageMut_;

#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
    // for unify rendering
    sptr<SurfaceBuffer> surfaceBuffer_;
    EGLImageKHR eglImage_ = EGL_NO_IMAGE_KHR;
    OH_NativeBuffer *nativeBuffer_ = nullptr;
    NativeWindowBuffer *nativeWindowBuffer_ = nullptr;
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_TEXTURE_LAYER_H
