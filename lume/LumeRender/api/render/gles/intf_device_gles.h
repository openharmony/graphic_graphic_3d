/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_GLES_IDEVICE_GLES_H
#define API_RENDER_GLES_IDEVICE_GLES_H

#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>

// Platform / Backend specific typedefs.
#if RENDER_HAS_GLES_BACKEND
#include <EGL/egl.h>
#elif RENDER_HAS_GL_BACKEND
#ifndef WINAPI
#define WINAPI __stdcall
using HANDLE = void*;
using HINSTANCE = struct HINSTANCE__*;
using HMODULE = HINSTANCE;
using HWND = struct HWND__*;
using HDC = struct HDC__*;
using HGLRC = struct HGLRC__*;
#endif
#endif

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_gfx_gles_idevicegles
 *  @{
 */
#if RENDER_HAS_GLES_BACKEND || DOXYGEN
/** Backend extra gles */
struct BackendExtraGLES : public BackendExtra {
    /** Application context, If != EGL_NO_CONTEXT the device/engine will use this EGLContext to do rendering.
        *WARNING* GLES state caching might cause issues in this case.
        (application can change engine GL state and engine can change application GL state)
    */
    EGLContext applicationContext { EGL_NO_CONTEXT };
    /** Shared context If != EGL_NO_CONTEXT the device/engine will create it's own context with context sharing enabled.
     */
    EGLContext sharedContext { EGL_NO_CONTEXT };
    /** Display to use */
    NativeDisplayType display { EGL_DEFAULT_DISPLAY };
    /** MSAA samples, 0 no MSAA, 4 = 4xMSAA backbuffer, 8 = 8X MSAA */
    uint32_t MSAASamples { 0 };
    /** Alpha bits, request 8 bits of alpha to backbuffer by default */
    uint32_t alphaBits { 8 };
    /** Depth bits, request NO depth buffer by default. */
    uint32_t depthBits { 24 };
    /** Stencil bits, request NO stencil buffer by default. */
    uint32_t stencilBits { 0 };
};

/** Device platform data gles */
struct DevicePlatformDataGLES : DevicePlatformData {
    /** Display */
    EGLDisplay display { EGL_NO_DISPLAY };
    /** Config */
    EGLConfig config { nullptr };
    /** Context */
    EGLContext context { EGL_NO_CONTEXT };
    /** EGL Version */
    uint32_t majorVersion { 0 };
    uint32_t minorVersion { 0 };
    /** Does EGL have EGL_KHR_gl_colorspace **/
    bool hasColorSpaceExt { false };
    /** Context created by us, also destroy it */
    bool contextCreated { false };
    /** Call eglInitialize/eglTerminate or not */
    bool eglInitialized { false };
};

/** The following structure can be used to pass a TextureObject from application GLES context to Engine Context
 * (device must be created with context sharing enabled or with app context) */
struct ImageDescGLES : BackendSpecificImageDesc {
    /** Type, GL_TEXTURE_2D / GL_TEXTURE_EXTERNAL_OES / etc */
    uint32_t type;
    /** Image, Texture handle (glGenTextures) */
    uint32_t image;
    /** Internal format, GL_RGBA16F etc */
    uint32_t internalFormat;
    /** Format, GL_RGB etc */
    uint32_t format;
    /** Data type, GL_FLOAT etc */
    uint32_t dataType;
    /** Bytes per pixel */
    uint32_t bytesperpixel;

    /** If non-zero should ba a valid EGLImage handle to be used as the source for the image. */
    uintptr_t eglImage { 0u };
};
#endif

#if RENDER_HAS_GL_BACKEND || DOXYGEN

/** Backend extra gl */
struct BackendExtraGL : public BackendExtra {
    /** MSAA samples, 0 no MSAA, 4 = 4xMSAA backbuffer, 8 = 8X MSAA */
    uint32_t MSAASamples { 0 };
    /** Alpha bits, request 8 bits of alpha to backbuffer by default */
    uint32_t alphaBits { 8 };
    /** Depth bits, request NO depth buffer by default. */
    uint32_t depthBits { 24 };
    /** Stencil bits, request NO stencil buffer by default. */
    uint32_t stencilBits { 0 };
};

/** Device platform data gl */
struct DevicePlatformDataGL : DevicePlatformData {
#if _WIN32 || DOXYGEN
    /** Hwnd */
    HWND mhWnd = NULL;
    /** Display */
    HDC display = NULL;
    /** Context */
    HGLRC context = NULL;
#endif
};

/** The following structure can be used to pass a TextureObject from application GLES context to Engine Context
 * (device must be created with context sharing enabled or with app context) */
struct ImageDescGL : BackendSpecificImageDesc {
    /** Type, GL_TEXTURE_2D / GL_TEXTURE_EXTERNAL_OES / etc */
    uint32_t type;
    /** Texture handle (glGenTextures) */
    uint32_t image;
    /** Internal format, GL_RGBA16F etc */
    uint32_t internalFormat;
    /** Format, GL_RGB etc */
    uint32_t format;
    /** Data type, GL_FLOAT etc */
    uint32_t dataType;
    /** Bytes per pixel */
    uint32_t bytesperpixel;
};
#endif

/** Provides interface for low-level access */
class ILowLevelDeviceGLES : public ILowLevelDevice {
public:
    // no-support for low level resource access
protected:
    ILowLevelDeviceGLES() = default;
    ~ILowLevelDeviceGLES() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_GLES_IDEVICE_GLES_H
