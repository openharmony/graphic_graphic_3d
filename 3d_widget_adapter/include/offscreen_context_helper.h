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
