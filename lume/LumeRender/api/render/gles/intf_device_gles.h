/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#if RENDER_HAS_GLES_BACKEND || (defined(DOXYGEN) && DOXYGEN)
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
    uint32_t depthBits { 0 };
    /** Stencil bits, request NO stencil buffer by default. */
    uint32_t stencilBits { 0 };
};

struct DevicePropertiesGLES {
    // ES20
    int32_t maxCombinedTextureImageUnits;
    int32_t maxCubeMapTextureSize;
    int32_t maxFragmentUniformVectors;
    int32_t maxRenderbufferSize;
    int32_t maxTextureImageUnits;
    int32_t maxTextureSize;
    int32_t maxVaryingVectors;
    int32_t maxVertexAttribs;
    int32_t maxVertexTextureImageUnits;
    int32_t maxVertexUniformVectors;
    float maxViewportDims[2];
    int32_t numCompressedTextureFormats;
    int32_t numShaderBinaryFormats;
    int32_t numProgramBinaryFormats;

    // ES30
    int32_t max3DTextureSize;
    int32_t maxArrayTextureLayers;
    int32_t maxColorAttachments;
    int64_t maxCombinedFragmentUniformComponents;
    int32_t maxCombinedUniformBlocks;
    int64_t maxCombinedVertexUniformComponents;
    int32_t maxDrawBuffers;
    int64_t maxElementIndex;
    int32_t maxElementsIndices;
    int32_t maxElementsVertices;
    int32_t maxFragmentInputComponents;
    int32_t maxFragmentUniformBlocks;
    int32_t maxFragmentUniformComponents;
    int32_t minProgramTexelOffset;
    int32_t maxProgramTexelOffset;
    int32_t maxSamples;
    int64_t maxServerWaitTimeout;
    float maxTextureLodBias;
    int32_t maxTransformFeedbackInterleavedComponents;
    int32_t maxTransformFeedbackSeparateAttribs;
    int32_t maxTransformFeedbackSeparateComponents;
    int64_t maxUniformBlockSize;
    int32_t maxUniformBufferBindings;
    int32_t maxVaryingComponents;
    int32_t maxVertexOutputComponents;
    int32_t maxVertexUniformBlocks;
    int32_t maxVertexUniformComponents;

    // ES31
    int32_t maxAtomicCounterBufferBindings;
    int32_t maxAtomicCounterBufferSize;
    int32_t maxColorTextureSamples;
    int32_t maxCombinedAtomicCounters;
    int32_t maxCombinedAtomicCounterBuffers;
    int32_t maxCombinedComputeUniformComponents;
    int32_t maxCombinedImageUniforms;
    int32_t maxCombinedShaderOutputResources;
    int32_t maxCombinedShaderStorageBlocks;
    int32_t maxComputeAtomicCounters;
    int32_t maxComputeAtomicCounterBuffers;
    int32_t maxComputeImageUniforms;
    int32_t maxComputeShaderStorageBlocks;
    int32_t maxComputeSharedMemorySize;
    int32_t maxComputeTextureImageUnits;
    int32_t maxComputeUniformBlocks;
    int32_t maxComputeUniformComponents;
    int32_t maxComputeWorkGroupCount[3];
    int32_t maxComputeWorkGroupInvocations;
    int32_t maxComputeWorkGroupSize[3];
    int32_t maxDepthTextureSamples;
    int32_t maxFragmentAtomicCounters;
    int32_t maxFragmentAtomicCounterBuffers;
    int32_t maxFragmentImageUniforms;
    int32_t maxFragmentShaderStorageBlocks;
    int32_t maxFramebufferHeight;
    int32_t maxFramebufferSamples;
    int32_t maxFramebufferWidth;
    int32_t maxImageUnits;
    int32_t maxIntegerSamples;
    int32_t minProgramTextureGatherOffset;
    int32_t maxProgramTextureGatherOffset;
    int32_t maxSampleMaskWords;
    int64_t maxShaderStorageBlockSize;
    int32_t maxShaderStorageBufferBindings;
    int32_t maxUniformLocations;
    int32_t maxVertexAtomicCounters;
    int32_t maxVertexAtomicCounterBuffers;
    int32_t maxVertexAttribBindings;
    int32_t maxVertexAttribRelativeOffset;
    int32_t maxVertexAttribStride;
    int32_t maxVertexImageUniforms;
    int32_t maxVertexShaderStorageBlocks;
    int32_t uniformBufferOffsetAlignment;
    int32_t shaderStorageBufferOffsetAlignment;

    // ES32
    int32_t minSampleShadingValue;
    int32_t maxDebugGroupStackDepth;
    int32_t maxDebugLoggedMessages;
    int32_t maxDebugMessageLength;
    float minFragmentInterpolationOffset;
    float maxFragmentInterpolationOffset;
    int32_t maxFramebufferLayers;
    int32_t maxLabelLength;
    int32_t maxTextureBufferSize;
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
    /** Device name reported by the driver. */
    const char* deviceName;
    /** Version string reported by the driver. */
    const char* driverVersion;
    /** Collection of implementation dependant limits. */
    DevicePropertiesGLES deviceProperties;
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
    /** Platform specific hardware buffer */
    uintptr_t platformHwBuffer { 0u };
};

struct BackendConfigGLES final : BackendConfig {
    /** GL_EXT_multisampled_render_to_texture2 says depth resolve is same as invalidation, but some implementations
     * actually do a depth resolve. Application should check DeviceName and DriverVersion to make sure it's running on a
     * device where it might work.*/
    bool allowDepthResolve;
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
    uint32_t depthBits { 0 };
    /** Stencil bits, request NO stencil buffer by default. */
    uint32_t stencilBits { 0 };
#if _WIN32 || DOXYGEN
    /** Handle to window. If no window is given, backend will try to use the current active window. */
    HWND window { nullptr };
    /** Shared context If != nullptr the device/engine will create it's own context with context sharing enabled. */
    HGLRC sharedContext { nullptr };
#endif
};

struct DevicePropertiesGL {
    int32_t max3DTextureSize;
    int32_t maxTextureSize;
    int32_t maxArrayTextureLayers;
    float maxTextureLodBias;
    int32_t maxCubeMapTextureSize;
    int32_t maxRenderbufferSize;
    float maxTextureMaxAnisotropy;
    float maxViewportDims[2];
    int32_t maxViewports;
    int32_t viewportSubpixelBits;
    int32_t viewportBoundsRange;

    int32_t majorVersion;
    int32_t minorVersion;
    int32_t numProgramBinaryFormats;
    int32_t numShaderBinaryFormats;

    int32_t maxVertexAttribs;
    int32_t maxVertexUniformComponents;
    int32_t maxVertexUniformVectors;
    int32_t maxVertexUniformBlocks;
    int32_t maxVertexImageUniforms;
    int32_t maxVertexOutputComponents;
    int32_t maxVertexTextureImageUnits;
    int32_t maxVertexAtomicCounterBuffers;
    int32_t maxVertexAtomicCounters;
    int32_t maxVertexShaderStorageBlocks;
    int32_t maxCombinedVertexUniformComponents;

    int32_t maxFragmentUniformComponents;
    int32_t maxFragmentUniformVectors;
    int32_t maxFragmentUniformBlocks;
    int32_t maxFragmentImageUniforms;
    int32_t maxFragmentInputComponents;
    int32_t maxFragmentImageUnits;
    int32_t maxFragmentAtomicCounterBuffers;
    int32_t maxFragmentAtomicCounters;
    int32_t maxFragmentShaderStorageBlocks;
    int32_t maxCombinedFragmentUniformComponents;

    int32_t maxComputeWorkGroupCount[3];
    int32_t maxComputeWorkGroupSize[3];
    int32_t maxComputeWorkGroupInvocations;
    int32_t maxComputeUniformBlocks;
    int32_t maxComputeImageUniforms;
    int32_t maxComputeTextureImageUnits;
    int32_t maxComputeAtomicCounterBuffers;
    int32_t maxComputeAtomicCounters;
    int32_t maxComputeSharedMemorySize;
    int32_t maxComputeUniformComponents;
    int32_t maxComputeShaderStorageBlocks;
    int32_t maxCombinedComputeUniformComponents;

    int32_t maxTextureBufferSize;
    int32_t minProgramTexelOffset;
    int32_t maxProgramTexelOffset;
    int32_t minProgramTextureGatherOffset;
    int32_t maxProgramTextureGatherOffset;
    int32_t maxUniformBufferBindings;
    int32_t maxUniformBlockSize;
    int32_t uniformBufferOffsetAlignment;
    int32_t maxCombinedUniformBlocks;
    int32_t maxUniformLocations;
    int32_t maxVaryingComponents;
    int32_t maxVaryingFloats;
    int32_t maxVaryingVectors;
    int32_t maxCombinedTextureImageUnits;
    int32_t maxAtomicCounterBufferBindings;
    int32_t maxAtomicCounterBufferSize;
    int32_t maxCombinedAtomicCounters;
    int32_t maxCombinedAtomicCounterBuffers;
    int32_t maxShaderStorageBufferBindings;
    int32_t maxShaderStorageBlockSize;
    int32_t maxCombinedShaderStorageBlocks;
    int32_t shaderStorageBufferOffsetAlignment;
    int32_t maxImageUnits;
    int32_t maxCombinedShaderOutputResources;
    int32_t maxCombinedImageUniforms;

    int32_t minMapBufferAlignment;
    int32_t maxVertexAttribRelativeOffset;
    int32_t maxVertexAttribBindings;
    int32_t maxVertexAttribStride;
    int32_t maxElementsIndices;
    int32_t maxElementsVertices;
    int64_t maxElementIndex;
    int32_t maxClipDistances;
    int32_t maxColorAttachments;
    int32_t maxFramebufferWidth;
    int32_t maxFramebufferHeight;
    int32_t maxFramebufferLayers;
    int32_t maxFramebufferSamples;
    int32_t maxSampleMaskWords;
    int32_t maxSamples;
    int32_t maxColorTextureSamples;
    int32_t maxDepthTextureSamples;
    int32_t maxIntegerSamples;
    int64_t maxServerWaitTimeout;
    int32_t maxDrawBuffers;
    int32_t maxDualSourceDrawBuffers;
    int32_t maxLabelLength;
};

/** Device platform data gl */
struct DevicePlatformDataGL : DevicePlatformData {
    /** Device name reported by the driver. */
    const char* deviceName;
    /** Version string reported by the driver. */
    const char* driverVersion;
    /** Collection of implementation dependant limits. */
    DevicePropertiesGL deviceProperties;
#if _WIN32 || DOXYGEN
    /** Hwnd */
    HWND mhWnd = nullptr;
    /** Display */
    HDC display = nullptr;
    /** Context */
    HGLRC context = nullptr;
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
#if RENDER_HAS_EXPERIMENTAL
    /** EXPERIMENTAL: Those methods have been added to be able to use opengl context initialized with lume render as a
       platform abstraction library.
       This should not be used while using standart render node graph rendering with lume.
       Manual activation of gles device. When activated, DevicePlatformData will contains valid pointers
       to the opengl context which will be made active on the current thread. All further call to GL must be from this
       thread until this device is release with an explitic call to Deactivate(). Device will be guarded internally by a
       mutex while activated. */
    virtual void Activate() = 0;
    /** EXPERIMENTAL: Manual deactivation of gles device following an explicit activation by a call to Activate() on the
     * same thread. */
    virtual void Deactivate() = 0;
    /** EXPERIMENTAL: Manual call to GL Swapbuffer with the current default swapchain if it exists. If no swapchain
       exists, this function has no effect. this call must apend between a call to Activate() and Deactivate().
    */
    virtual void SwapBuffers() = 0;
// no-support for low level resource access
#endif
protected:
    ILowLevelDeviceGLES() = default;
    ~ILowLevelDeviceGLES() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_GLES_IDEVICE_GLES_H
