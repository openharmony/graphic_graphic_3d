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

#include "offscreen_context_helper.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <window.h>

#include <3d_widget_adapter_log.h>


namespace OHOS::Render3D {
AutoRestore::AutoRestore()
{
    c_ = eglGetCurrentContext();
    rs_ = eglGetCurrentSurface(EGL_READ);
    ws_ = eglGetCurrentSurface(EGL_DRAW);
    d_ = eglGetCurrentDisplay();
    if (d_ == EGL_NO_DISPLAY) {
        d_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (!eglMakeCurrent(d_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        WIDGET_LOGE("AutoRestore make current error %d", eglGetError());
    }
}

AutoRestore::~AutoRestore()
{
    if (!eglMakeCurrent(d_, ws_, rs_, c_)) {
        WIDGET_LOGE("~AutoRestore make current error %d", eglGetError());
    }
}

OffScreenContextHelper::~OffScreenContextHelper()
{
    DestroyOffScreenContext();
}

EGLContext OffScreenContextHelper::CreateOffScreenContext(EGLContext eglContext)
{
    if (localThreadContext_ != EGL_NO_CONTEXT) {
        return localThreadContext_;
    }

    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglContext == EGL_NO_CONTEXT) {
        EGLint major = 0;
        EGLint minor = 0;
        if (!eglInitialize(eglDisplay_, &major, &minor)) {
            WIDGET_LOGW("Initialize egl fail");
        }
        if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
            WIDGET_LOGW("faile to bind gles api");
        }
    }

    EGLint contextAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 2,
        EGL_NONE, EGL_NONE,
    };
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT, EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER, EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_CONFORMANT, EGL_OPENGL_ES3_BIT_KHR, EGL_NONE, EGL_NONE,
    };
    EGLint count = -1;
    EGLConfig config;

    if (!eglChooseConfig(eglDisplay_, configAttribs, &config, 1, &count)) {
        WIDGET_LOGE("eglChooseConfig error %d", eglGetError());
        return EGL_NO_CONTEXT;
    }

    localThreadContext_ = eglCreateContext(eglDisplay_, config, eglContext, contextAttrs);
    auto error = eglGetError();
    if (error != EGL_SUCCESS) {
        WIDGET_LOGE("create context fail %d", error);
        return EGL_NO_CONTEXT;
    }

    EGLint pbufferAttribs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
    };
    pBufferSurface_ = eglCreatePbufferSurface(eglDisplay_, config, pbufferAttribs);
    return localThreadContext_;
}

EGLContext OffScreenContextHelper::GetOffScreenContext()
{
    return localThreadContext_;
}

void OffScreenContextHelper::BindOffScreenContext()
{
    if (!eglMakeCurrent(eglDisplay_, pBufferSurface_, pBufferSurface_, localThreadContext_)) {
        WIDGET_LOGE("%s, eglMakecurrent error %d", __func__, eglGetError());
    }
}

void OffScreenContextHelper::DestroyOffScreenContext()
{
    WIDGET_LOGI("DestroyOffScreenContext");
    if (pBufferSurface_ != EGL_NO_SURFACE) {
        eglDestroySurface(eglDisplay_, pBufferSurface_);
        pBufferSurface_ = EGL_NO_SURFACE;
    }

    if (localThreadContext_ != EGL_NO_CONTEXT) {
        eglDestroyContext(eglDisplay_, localThreadContext_);
        localThreadContext_ = EGL_NO_CONTEXT;
    }
}
} // namespace OHOS::Render3D
