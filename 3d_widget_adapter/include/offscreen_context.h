/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_OFFSCREEN_CONTEXT_H
#define OHOS_RENDER_3D_OFFSCREEN_CONTEXT_H

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace OHOS::Render3D {
class OffScreenContext final {
public:
    OffScreenContext() = default;
    ~OffScreenContext();
    EGLContext CreateOffScreenContext(EGLContext eglContext);
    EGLContext GetOffScreenContext();
    void TurnOnOffScreenContext();
    void DestroyOffScreenContext();
    GLuint CreateRenderTarget(EGLContext eglContext, uint32_t width, uint32_t height);
    void DestroyRenderTarget(GLuint texture);
private:
    class AutoRestore final {
    public:
        AutoRestore();
        ~AutoRestore();
    private:
        EGLContext c_ = EGL_NO_CONTEXT;
        EGLSurface ws_ = EGL_NO_SURFACE;
        EGLSurface rs_ = EGL_NO_SURFACE;
        EGLDisplay d_ = EGL_NO_DISPLAY;
    };
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLContext localThreadContext_ = EGL_NO_CONTEXT;
    EGLSurface pBufferSurface_ = EGL_NO_SURFACE;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_OFFSCREEN_CONTEXT_H
