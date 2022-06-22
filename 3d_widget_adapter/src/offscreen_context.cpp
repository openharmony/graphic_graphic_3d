/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "offscreen_context.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <3d_widget_adapter_log.h>
#include <string>

namespace OHOS::Render3D {
OffScreenContext::AutoRestore::AutoRestore()
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

OffScreenContext::AutoRestore::~AutoRestore()
{
    if (!eglMakeCurrent(d_, ws_, rs_, c_)) {
        WIDGET_LOGE("~AutoRestore make current error %d", eglGetError());
    }
}

OffScreenContext::~OffScreenContext()
{
    DestroyOffScreenContext();
}

EGLContext OffScreenContext::CreateOffScreenContext(EGLContext eglContext)
{
    AutoRestore scope;
    WIDGET_LOGE("CreateOffScreenContext");
    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint contextAttrs[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
        EGL_CONTEXT_MINOR_VERSION_KHR, 2,
        EGL_NONE, EGL_NONE,
    };

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_COLOR_BUFFER_TYPE,
        EGL_RGB_BUFFER,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES3_BIT_KHR,
        EGL_CONFORMANT,
        EGL_OPENGL_ES3_BIT_KHR,
        EGL_NONE,
        EGL_NONE,
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
        WIDGET_LOGE("create context %p fail %d",
            localThreadContext_, error);
        return EGL_NO_CONTEXT;
    }

    EGLint pbufferAttribs[] = {
        EGL_WIDTH, static_cast<int>(1),
        EGL_HEIGHT, static_cast<int>(1),
        EGL_NONE,
    };

    pBufferSurface_ = eglCreatePbufferSurface(eglDisplay_, config, pbufferAttribs);

    return localThreadContext_;
}


EGLContext OffScreenContext::GetOffScreenContext()
{
    return localThreadContext_;
}

void OffScreenContext::TurnOnOffScreenContext()
{
    if (!eglMakeCurrent(eglDisplay_, pBufferSurface_, pBufferSurface_, localThreadContext_)) {
        WIDGET_LOGE("%s, eglMakecurrent error %d", __func__, eglGetError());
    }
}

GLuint OffScreenContext::CreateRenderTarget(EGLContext platformContext, uint32_t width, uint32_t height)
{
    AutoRestore scope;
    if (localThreadContext_ == EGL_NO_CONTEXT) {
        CreateOffScreenContext(platformContext);
    }
    TurnOnOffScreenContext();
    GLuint texId;
    glGenTextures(1, &texId);

    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::string p;
    for (int i = 0; i < width * height; i++) {
        p.push_back(255); // red
        p.push_back(0);
        p.push_back(0);
        p.push_back(255); // alpha
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, p.c_str());

    return texId;
}

void OffScreenContext::DestroyRenderTarget(GLuint texture)
{
    AutoRestore scope;
    TurnOnOffScreenContext();
    glDeleteTextures(1, &texture);
}

void OffScreenContext::DestroyOffScreenContext()
{
    AutoRestore scope;
    WIDGET_LOGE("DestroyOffScreenContext");
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
