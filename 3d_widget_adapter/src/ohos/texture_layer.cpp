/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "texture_layer.h"

#include <base/log/ace_trace.h>
#include <include/gpu/GrBackendSurface.h>
#ifndef NEW_SKIA
#include <include/gpu/GrContext.h>
#else
#include <include/gpu/GrDirectContext.h>
#endif
#include <include/gpu/gl/GrGLInterface.h>
#include <native_buffer.h>
#include <render_service_base/include/pipeline/rs_recording_canvas.h>
#include <surface_buffer.h>
#include <window.h>

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "offscreen_context_helper.h"

namespace OHOS {
namespace Render3D {

void TextureLayer::UpdateRenderFinishFuture(std::shared_future<void> &ftr)
{
    std::lock_guard<std::mutex> lk(ftrMut_);
    image_.ftr_ = ftr;
}

void TextureLayer::SetOffset(int32_t x, int32_t y)
{
    offsetX_ = x;
    offsetY_ = y;
}

void TextureLayer::SetTextureInfo(const TextureInfo &info)
{
    image_.textureInfo_ = info;
}

void TextureLayer::AllocGLTexture(uint32_t width, uint32_t height)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, 0);
}

void TextureLayer::AllocEglImage(uint32_t width, uint32_t height)
{
    OH_NativeBuffer_Config config {
        .width = width,
        .height = height,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_MEM_DMA
    };

    nativeBuffer_ = OH_NativeBuffer_Alloc(&config);
    if (!nativeBuffer_) {
        WIDGET_LOGE("native buffer null");
        return;
    }

    surfaceBuffer_ = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer_);
    if (!surfaceBuffer_) {
        WIDGET_LOGE("surface buffer null");
        FreeNativeBuffer();
        return;
    }

    nativeWindowBuffer_ = CreateNativeWindowBufferFromSurfaceBuffer(&surfaceBuffer_);
    if (!nativeWindowBuffer_) {
        WIDGET_LOGE("create native window buffer fail");
        FreeNativeBuffer();
        return;
    }

    EGLint attrs[] = {
        EGL_IMAGE_PRESERVED, EGL_TRUE, EGL_NONE,
    };
    auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglImage_ = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS, nativeWindowBuffer_, attrs);
    if (eglImage_ == EGL_NO_IMAGE_KHR) {
        WIDGET_LOGE("create egl image fail %d", eglGetError());
        FreeNativeWindowBuffer();
        FreeNativeBuffer();
        return;
    }

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(eglImage_));
}

TextureInfo TextureLayer::CreateRenderTarget(uint32_t width, uint32_t height)
{
    AutoRestore scope;

    GraphicsManager::GetInstance().BindOffScreenContext();
    GLuint texId = 0U;
    glGenTextures(1, &texId);
    TextureInfo textureInfo { width, height, texId };

    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    AllocGLTexture(width, height);

#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
    AllocEglImage(width, height);
#endif

    return textureInfo;
}

TextureLayer::~TextureLayer()
{
    DestroyRenderTarget();
}

void TextureLayer::FreeNativeBuffer()
{
    if (!nativeBuffer_) {
        return;
    }
    OH_NativeBuffer_Unreference(nativeBuffer_);
    nativeBuffer_ = nullptr;
}

void TextureLayer::FreeNativeWindowBuffer()
{
    if (!nativeWindowBuffer_) {
        return;
    }
    DestroyNativeWindowBuffer(nativeWindowBuffer_);
    nativeWindowBuffer_ = nullptr;
}

void TextureLayer::DestroyRenderTarget()
{
    if (eglImage_ != EGL_NO_IMAGE_KHR) {
        auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglDestroyImageKHR(disp, eglImage_);
        eglImage_ = EGL_NO_IMAGE_KHR;
    }

    FreeNativeWindowBuffer();
    FreeNativeBuffer();

    if (image_.textureInfo_.textureId_ != 0U) {
        AutoRestore scope;
        GraphicsManager::GetInstance().BindOffScreenContext();
        glDeleteTextures(1, &image_.textureInfo_.textureId_);
    }
    image_.textureInfo_ = {};

#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
    if (fbo_ != 0U) {
        AutoRestore scope;
        GraphicsManager::GetInstance().BindOffScreenContext();
        glDeleteFramebuffers(1, fbo_);
        fbo_ = 0U;
    }

    if (data_ != nullptr) {
        delete[] data_;
        data_ = nullptr;
    }
#endif
}

SkRect TextureLayer::onGetBounds()
{
    return SkRect::MakeWH(image_.textureInfo_.width_, image_.textureInfo_.height_);
}

void TextureLayer::OnDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        WIDGET_LOGE("TextureLayer::OnDraw canvas invalid")
        return;
    };

    #if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
        onDraw(canvas);
    #else
        canvas->drawDrawable(this);
    #endif
}

void TextureLayer::onDraw(SkCanvas* canvas)
{
    OHOS::Ace::ACE_SCOPED_TRACE("TextureLayer::onDraw");
    if (canvas == nullptr) {
        WIDGET_LOGE("TextureLayer::OnDraw canvas invalid")
        return;
    };
    std::lock_guard<std::mutex> lk(ftrMut_);
    if (image_.ftr_.valid()) {
        image_.ftr_.get();
    }
    #if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
        DrawTextureUnifyRender(canvas);
    #else
        DrawTexture(canvas);
    #endif
}

#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
auto TextureLayer::MakePixelImage()
{
    SkColorType ct = SkColorType::kRGBA_8888_SkColorType;
    SkAlphaType at = SkAlphaType::kOpaque_SkAlphaType;
    sk_sp<SkColorSpace> cs = SkColorSpace::MakeSRGB();
    auto info = SkImageInfo::Make(image_.textureInfo_.width_, image_.textureInfo_.height_, ct, at, cs);

    SkPixmap imagePixmap(info, reinterpret_cast<const void*>(data_), image_.textureInfo_.width_ * 4);

    return SkImage::MakeFromRaster(imagePixmap, nullptr, nullptr);
}

void TextureLayer::ReadPixel()
{
    AutoRestore scope;
    GraphicsManager::GetInstance().BindOffScreenContext();

    if (fbo_ == 0) {
        CreateReadFbo();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    if (data_ == nullptr) {
        data_ = new uint8_t[image_.textureInfo_.width_ * image_.textureInfo_.height_ * 4];
    }

    glReadPixels(0, 0, image_.textureInfo_.width_, image_.textureInfo_.height_, GL_RGBA, GL_UNSIGNED_BYTE, data_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureLayer::CreateReadFbo()
{
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glBindTexture(GL_TEXTURE_2D, image_.textureInfo_.textureId_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image_.textureInfo_.textureId_, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        WIDGET_LOGE("framebuffer not complete, status %d, fbo_ %d, textureId_ %d",
            status, fbo_, image_.textureInfo_.textureId_);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif

#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
void TextureLayer::DrawTextureUnifyRender(SkCanvas* canvas)
{
#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
    // DEBUG copy texture to bitmap, no need ipc surfacebuffer
    ReadPixel();
    canvas->drawImage(MakePixelImage(), offsetX_, offsetY_);
    return;
#endif
    Rosen::RSSurfaceBufferInfo info {
        surfaceBuffer_, offsetX_, offsetY_, width_, height_
    };
    auto recordingCanvas = static_cast<Rosen::RSRecordingCanvas*>(canvas);
    recordingCanvas->DrawSurfaceBuffer(info);
}
#endif

void TextureLayer::DrawTexture(SkCanvas* canvas)
{
    OHOS::Ace::ACE_SCOPED_TRACE("TextureLayer::DrawTexture");
    if (image_.textureInfo_.textureId_ <= 0) {
        WIDGET_LOGE("%s invalid texture %d", __func__, __LINE__);
        return;
    }

    if (image_.skImage_ == nullptr
        || image_.textureInfo_.width_ != width_
        || image_.textureInfo_.height_ != height_) {
        image_.textureInfo_.width_ = width_;
        image_.textureInfo_.height_ = height_;
        GrGLTextureInfo textureInfo = { GL_TEXTURE_2D, image_.textureInfo_.textureId_, GL_RGBA8_OES };
        GrBackendTexture backendTexture(image_.textureInfo_.width_, image_.textureInfo_.height_,
            GrMipMapped::kNo, textureInfo);
#ifdef NEW_SKIA
        image_.skImage_ = SkImage::MakeFromTexture(canvas->recordingContext(), backendTexture, kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
#else
        image_.skImage_ = SkImage::MakeFromTexture(canvas->getGrContext(), backendTexture, kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
#endif
        WIDGET_LOGW("%s Create SkImage %d", __func__, __LINE__);
    }

    canvas->drawImage(image_.skImage_, offsetX_, offsetY_);
}

void TextureLayer::SetWH(uint32_t w, uint32_t h)
{
    width_ = w;
    height_ = h;
}
}
}
