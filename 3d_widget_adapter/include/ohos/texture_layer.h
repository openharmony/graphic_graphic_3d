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

#include <include/core/SkCanvas.h>
#include <include/core/SkDrawable.h>
#include <include/core/SkImage.h>

#include <render_service_client/core/ui/rs_node.h>
#include <surface.h>
#include <surface_buffer.h>

#include "data_type/constants.h"
#include "graphics_manager.h"
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

struct WindowChangeInfo {
    float offsetX = 0.0;
    float offsetY = 0.0;
    float width = 0.0;
    float height = 0.0;
    float scale = 1.0;
    float widthScale = 1.0;
    float heightScale = 1.0;
    bool recreateWindow = true;
    SurfaceType surfaceType = SurfaceType::SURFACE_TEXTURE;
};

class __attribute__((visibility("default"))) TextureLayer : public SkDrawable {
public:
    explicit TextureLayer(int32_t key);
    virtual ~TextureLayer();

    void DestroyRenderTarget();
    virtual SkRect onGetBounds() override;
    void OnDraw(SkCanvas* canvas);
    virtual void onDraw(SkCanvas* canvas) override;
    TextureInfo GetTextureInfo();

    void SetParent(std::shared_ptr<Rosen::RSNode>& parent);
    TextureInfo OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
        bool recreateWindow, SurfaceType surfaceType = SurfaceType::SURFACE_WINDOW);
    TextureInfo OnWindowChange(const WindowChangeInfo& windowChangeInfo);

private:
    void DrawTexture(SkCanvas* canvas);
    void* CreateNativeWindow(uint32_t width, uint32_t height);
#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
    void FreeNativeBuffer();
    void FreeNativeWindowBuffer();
    void DrawTextureUnifyRender(SkCanvas* canvas);
#endif
    void AllocGLTexture(uint32_t width, uint32_t height);
    void AllocEglImage(uint32_t width, uint32_t height);
    void DestroyProducerSurface();
    void DestroyNativeWindow();
    void ConfigWindow(float offsetX, float offsetY, float width, float height, float scale, bool recreateWindow);
    void ConfigTexture(float width, float height);
    // deprecated
    void UpdateRenderFinishFuture(std::shared_future<void> &ftr);

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
    int32_t key_ = INT32_MAX;
    bool needsRecreateSkImage_ = false;
    std::mutex ftrMut_;
    std::mutex skImageMut_;

    std::shared_ptr<Rosen::RSNode> rsNode_;
    sptr<OHOS::Surface> producerSurface_ = nullptr;
    RenderBackend backend_ = RenderBackend::UNDEFINE;
    SurfaceType surface_ = SurfaceType::UNDEFINE;

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
