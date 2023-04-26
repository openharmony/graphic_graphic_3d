/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_OFFSCREEN_CONTEXT_HELPER_H
#define OHOS_RENDER_3D_OFFSCREEN_CONTEXT_HELPER_H

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace OHOS::Render3D {
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

class OffScreenContextHelper final {
public:
    OffScreenContextHelper() = default;
    ~OffScreenContextHelper();
    EGLContext CreateOffScreenContext(EGLContext eglContext);
    EGLContext GetOffScreenContext();
    void BindOffScreenContext();
    void DestroyOffScreenContext();

private:
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLContext localThreadContext_ = EGL_NO_CONTEXT;
    EGLSurface pBufferSurface_ = EGL_NO_SURFACE;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_OFFSCREEN_CONTEXT_H
