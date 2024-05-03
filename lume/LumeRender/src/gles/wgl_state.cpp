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

#if RENDER_HAS_GL_BACKEND
#include "gles/wgl_state.h"

#include <base/containers/fixed_string.h>
#include <render/namespace.h>

#include "util/log.h"

// NOTE: intentional double include of gl_functions.h
#include "gles/gl_functions.h"
#define declare(a, b) a b = nullptr;
#include <gl/wgl.h>

#include "gles/gl_functions.h"
#include "gles/surface_information.h"
#include "gles/swapchain_gles.h"

using BASE_NS::string_view;
using BASE_NS::vector;

RENDER_BEGIN_NAMESPACE()
namespace WGLHelpers {
namespace {
#define WGLFUNCS                                                                   \
    declareWGL(PFNWGLGETPROCADDRESSPROC, wglGetProcAddress);                       \
    declareWGL(PFNWGLCREATECONTEXTPROC, wglCreateContext);                         \
    declareWGL(PFNWGLDELETECONTEXTPROC, wglDeleteContext);                         \
    declareWGL(PFNWGLMAKECURRENTPROC, wglMakeCurrent);                             \
    declareWGL(PFNWGLGETCURRENTCONTEXTPROC, wglGetCurrentContext);                 \
    declareWGL(PFNWGLGETCURRENTDCPROC, wglGetCurrentDC);                           \
    declareWGL(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);     \
    declareWGL(PFNWGLGETPIXELFORMATATTRIBIVARBPROC, wglGetPixelFormatAttribivARB); \
    declareWGL(PFNWGLGETPIXELFORMATATTRIBFVARBPROC, wglGetPixelFormatAttribfvARB); \
    declareWGL(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);           \
    declareWGL(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);                     \
    declareWGL(PFNWGLGETEXTENSIONSSTRINGARBPROC, wglGetExtensionsStringARB);

#define declareWGL(a, b) a b = nullptr;
WGLFUNCS;
#undef declareWGL

constexpr int WGL_ATTRIBS[] = {
#if defined(CORE_CREATE_GLES_CONTEXT_WITH_WGL) && (CORE_CREATE_GLES_CONTEXT_WITH_WGL == 1)
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2,
#if RENDER_GL_DEBUG
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_ES2_PROFILE_BIT_EXT, 0
#else
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB, 5,
#if RENDER_GL_DEBUG
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
#endif
};

static bool FilterError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const string_view message, const void* userParam) noexcept
{
    if (source == GL_DEBUG_SOURCE_API) {
        if (type == GL_DEBUG_TYPE_OTHER) {
            if ((id == 11) && (severity == GL_DEBUG_SEVERITY_LOW)) {
                /*  Ignore the following warning that Intel drivers seem to spam. (no problems on nvidia or mali...)
                source: GL_DEBUG_SOURCE_API
                type: GL_DEBUG_TYPE_OTHER
                severity: GL_DEBUG_SEVERITY_LOW
                message: API_ID_SYNC_FLUSH other warning has been generated. ClientWait flush in different gc for sync
                object 2, "" will be ineffective.
                */
                return true;
            }

            if ((id == 131185) && (severity == GL_DEBUG_SEVERITY_NOTIFICATION)) {
                /* Ignore this warning that Nvidia sends.
                source:      GL_DEBUG_SOURCE_API
                type:        GL_DEBUG_TYPE_OTHER
                severity:    GL_DEBUG_SEVERITY_NOTIFICATION
                message:     Buffer detailed info: Buffer object X (bound to GL_COPY_WRITE_BUFFER_BINDING_EXT, usage
                hint is GL_DYNAMIC_DRAW) will use SYSTEM HEAP memory as the source for buffer object operations. *OR*
                message:     Buffer detailed info: Buffer object X (bound to GL_COPY_WRITE_BUFFER_BINDING_EXT, usage
                hint is GL_DYNAMIC_DRAW) has been mapped WRITE_ONLY in SYSTEM HEAP memory (fast).
                */
                return true;
            }
        }
        if (type == GL_DEBUG_TYPE_PERFORMANCE) {
            if ((id == 131154) && (severity == GL_DEBUG_SEVERITY_MEDIUM)) {
                /*
                source: GL_DEBUG_SOURCE_API
                type: GL_DEBUG_TYPE_PERFORMANCE
                id: 131154
                severity: GL_DEBUG_SEVERITY_MEDIUM
                message: Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering.
                */
                return true;
            }
            if ((id == 131186) && (severity == GL_DEBUG_SEVERITY_MEDIUM)) {
                /* Ignore this warning that Nvidia sends.
                source: GL_DEBUG_SOURCE_API
                type: GL_DEBUG_TYPE_PERFORMANCE
                id: 131186
                severity: GL_DEBUG_SEVERITY_MEDIUM
                message: Buffer performance warning: Buffer object X (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint
                is GL_DYNAMIC_DRAW) is being copied/moved from VIDEO memory to HOST memory.
                */
                return true;
            }
            if ((id == 131202) && (severity == GL_DEBUG_SEVERITY_MEDIUM)) {
                /* Ignore this warning that Nvidia sends.
                source: GL_DEBUG_SOURCE_API
                type: GL_DEBUG_TYPE_PERFORMANCE
                id: 131202
                severity: GL_DEBUG_SEVERITY_MEDIUM
                message: Texture state performance warning: emulating compressed format not supported in hardware with
                decompressed images
                */
                return true;
            }
        }
    }
    return false;
}

static void DoDummy()
{
    WNDCLASS wc = { 0 };
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc; // WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = TEXT("PureGL_WGL_Dummy");
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);
    const HWND dummyhWnd = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPED, CW_USEDEFAULT,
        CW_USEDEFAULT, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
    const HDC dummyhDC = GetDC(dummyhWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;
    SetPixelFormat(dummyhDC, ChoosePixelFormat(dummyhDC, &pfd), &pfd);
    const HGLRC dummy = wglCreateContext(dummyhDC);
    wglMakeCurrent(dummyhDC, dummy);
#define declareWGL(a, b)                                                                    \
    if ((b) == nullptr) {                                                                   \
        *(reinterpret_cast<void**>(&(b))) = reinterpret_cast<void*>(wglGetProcAddress(#b)); \
    }
    WGLFUNCS
#undef declareWGL
    wglMakeCurrent(dummyhDC, nullptr);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy);
    ReleaseDC(dummyhWnd, dummyhDC);
    DestroyWindow(dummyhWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
}

void ParseExtensions(const string_view extensions, vector<string_view>& extensionList)
{
    size_t start = 0;
    for (auto end = extensions.find(' '); end != BASE_NS::string::npos; end = extensions.find(' ', start)) {
        extensionList.emplace_back(extensions.data() + start, end - start);
        start = end + 1;
    }
    if (start < extensions.size()) {
        extensionList.emplace_back(extensions.data() + start);
    }
}

void FillProperties(DevicePropertiesGL& properties)
{
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &properties.max3DTextureSize);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &properties.maxTextureSize);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &properties.maxArrayTextureLayers);
    glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &properties.maxTextureLodBias);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &properties.maxTextureMaxAnisotropy);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &properties.maxCubeMapTextureSize);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &properties.maxRenderbufferSize);
    glGetFloati_v(GL_MAX_VIEWPORT_DIMS, 0, properties.maxViewportDims);
    glGetFloati_v(GL_MAX_VIEWPORT_DIMS, 1, properties.maxViewportDims + 1);
    glGetIntegerv(GL_MAX_VIEWPORTS, &properties.maxViewports);
    glGetIntegerv(GL_VIEWPORT_SUBPIXEL_BITS, &properties.viewportSubpixelBits);
    glGetIntegerv(GL_VIEWPORT_BOUNDS_RANGE, &properties.viewportBoundsRange);

    glGetIntegerv(GL_MAJOR_VERSION, &properties.majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &properties.minorVersion);
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &properties.numProgramBinaryFormats);
    glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &properties.numShaderBinaryFormats);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &properties.maxVertexAttribs);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &properties.maxVertexUniformComponents);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &properties.maxVertexUniformVectors);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &properties.maxVertexUniformBlocks);
    glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &properties.maxVertexImageUniforms);
    glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &properties.maxVertexOutputComponents);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &properties.maxVertexTextureImageUnits);
    glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &properties.maxVertexAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &properties.maxVertexAtomicCounters);
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &properties.maxVertexShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, &properties.maxCombinedVertexUniformComponents);

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &properties.maxFragmentUniformComponents);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &properties.maxFragmentUniformVectors);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &properties.maxFragmentUniformBlocks);
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &properties.maxFragmentImageUniforms);
    glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &properties.maxFragmentInputComponents);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &properties.maxFragmentImageUnits);
    glGetIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.minProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.maxProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &properties.maxFragmentAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &properties.maxFragmentAtomicCounters);
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &properties.maxFragmentShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, &properties.maxCombinedFragmentUniformComponents);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, properties.maxComputeWorkGroupCount);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, properties.maxComputeWorkGroupCount + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, properties.maxComputeWorkGroupCount + 2);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, properties.maxComputeWorkGroupSize);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, properties.maxComputeWorkGroupSize + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, properties.maxComputeWorkGroupSize + 2);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &properties.maxComputeWorkGroupInvocations);
    glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &properties.maxComputeUniformBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &properties.maxComputeImageUniforms);
    glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &properties.maxComputeTextureImageUnits);
    glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, &properties.maxComputeAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &properties.maxComputeAtomicCounters);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &properties.maxComputeSharedMemorySize);
    glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &properties.maxComputeUniformComponents);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &properties.maxComputeShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, &properties.maxCombinedComputeUniformComponents);

    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &properties.maxTextureBufferSize);
    glGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &properties.minProgramTexelOffset);
    glGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &properties.maxProgramTexelOffset);
    glGetIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.minProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.maxProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &properties.maxUniformBufferBindings);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &properties.maxUniformBlockSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &properties.uniformBufferOffsetAlignment);
    glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &properties.maxCombinedUniformBlocks);
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &properties.maxUniformLocations);
    glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &properties.maxVaryingComponents);
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &properties.maxVaryingFloats);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &properties.maxVaryingVectors);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &properties.maxCombinedTextureImageUnits);
    glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &properties.maxAtomicCounterBufferBindings);
    glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, &properties.maxAtomicCounterBufferSize);
    glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTERS, &properties.maxCombinedAtomicCounters);
    glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, &properties.maxCombinedAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &properties.maxShaderStorageBufferBindings);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &properties.maxShaderStorageBlockSize);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &properties.maxCombinedShaderStorageBlocks);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &properties.shaderStorageBufferOffsetAlignment);
    glGetIntegerv(GL_MAX_IMAGE_UNITS, &properties.maxImageUnits);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &properties.maxCombinedShaderOutputResources);
    glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, &properties.maxCombinedImageUniforms);

    glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &properties.minMapBufferAlignment);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &properties.maxVertexAttribRelativeOffset);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &properties.maxVertexAttribBindings);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &properties.maxVertexAttribStride);
    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &properties.maxElementsIndices);
    glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &properties.maxElementsVertices);
    glGetInteger64v(GL_MAX_ELEMENT_INDEX, &properties.maxElementIndex);
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &properties.maxClipDistances);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &properties.maxColorAttachments);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &properties.maxFramebufferWidth);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &properties.maxFramebufferHeight);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &properties.maxFramebufferLayers);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &properties.maxFramebufferSamples);
    glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &properties.maxSampleMaskWords);
    glGetIntegerv(GL_MAX_SAMPLES, &properties.maxSamples);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &properties.maxColorTextureSamples);
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &properties.maxDepthTextureSamples);
    glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &properties.maxIntegerSamples);
    glGetInteger64v(GL_MAX_SERVER_WAIT_TIMEOUT, &properties.maxServerWaitTimeout);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &properties.maxDrawBuffers);
    glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &properties.maxDualSourceDrawBuffers);
    glGetIntegerv(GL_MAX_LABEL_LENGTH, &properties.maxLabelLength);
}
} // namespace

uintptr_t WGLState::CreateSurface(uintptr_t window, uintptr_t instance) const noexcept
{
    return reinterpret_cast<uintptr_t>(GetDC(reinterpret_cast<HWND>(window)));
}

void WGLState::DestroySurface(uintptr_t surface) const noexcept
{
    auto deviceContext = reinterpret_cast<HDC>(surface);
    HWND window = WindowFromDC(deviceContext);
    ReleaseDC(window, deviceContext);
}

bool WGLState::GetInformation(HDC surface, int32_t configId, GlesImplementation::SurfaceInfo& res) const
{
    if (configId > 0) {
        int attribList[16] = { WGL_RED_BITS_ARB, /* 0 */
            WGL_GREEN_BITS_ARB,                  /* 1 */
            WGL_BLUE_BITS_ARB,                   /* 2 */
            WGL_ALPHA_BITS_ARB,                  /* 3 */
            WGL_DEPTH_BITS_ARB,                  /* 4 */
            WGL_STENCIL_BITS_ARB,                /* 5 */
            WGL_SAMPLES_ARB };                   /* 6 */
        int32_t attribCnt = 7;
        int32_t colorspaceIndex = 0;
        int32_t srgbIndex = 0;
        if (hasColorSpace_) {
            colorspaceIndex = attribCnt;
            attribList[attribCnt] = WGL_COLORSPACE_EXT;
            attribCnt++;
        }
        if (hasSRGBFB_) {
            srgbIndex = attribCnt;
            attribList[attribCnt] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
            attribCnt++;
        }
        int values[16] {};
        const bool ret =
            wglGetPixelFormatAttribivARB(surface, configId, 0, static_cast<UINT>(attribCnt), attribList, values);
        if (ret) {
            res.configId = static_cast<uint32_t>(configId);
            res.red_size = static_cast<uint32_t>(values[0]);
            res.green_size = static_cast<uint32_t>(values[1]);
            res.blue_size = static_cast<uint32_t>(values[2]);
            res.alpha_size = static_cast<uint32_t>(values[3]);
            res.depth_size = static_cast<uint32_t>(values[4]);
            res.stencil_size = static_cast<uint32_t>(values[5]);
            res.samples = static_cast<uint32_t>(values[6]);
            res.srgb = false; // default to linear framebuffer
            if (hasSRGBFB_) {
                // is framebuffer srgb capable?
                res.srgb = (values[srgbIndex] != 0) ? true : false;
            }
            if (hasColorSpace_) {
                // has srgb colorspace?
                res.srgb = static_cast<uint32_t>(values[colorspaceIndex]) == WGL_COLORSPACE_SRGB_EXT;
            }
            return true;
        }
    }
    return false;
}

bool WGLState::GetSurfaceInformation(HDC surface, GlesImplementation::SurfaceInfo& res) const
{
    RECT rcCli;
    GetClientRect(WindowFromDC(surface), &rcCli);
    // then you might have:
    res.width = static_cast<uint32_t>(rcCli.right - rcCli.left);
    res.height = static_cast<uint32_t>(rcCli.bottom - rcCli.top);
    int32_t configId = GetPixelFormat(surface);
    if (!configId) {
        configId = GetPixelFormat(plat_.display);
        if (configId) {
            PIXELFORMATDESCRIPTOR pfd = { 0 };
            DescribePixelFormat(plat_.display, configId, sizeof(pfd), &pfd);
            SetPixelFormat(surface, configId, &pfd);
        }
    }
    return GetInformation(surface, configId, res);
}

void WGLState::SwapBuffers(const SwapchainGLES& swapChain)
{
    auto& plat = swapChain.GetPlatformData();
    ::SwapBuffers(reinterpret_cast<HDC>(plat.surface));
}

bool WGLState::HasExtension(const string_view extension) const
{
    for (const auto& e : extensionList_) {
        if (extension == e)
            return true;
    }
    return false;
}

int WGLState::ChoosePixelFormat(HDC dc, const vector<int>& attributes)
{
    /*
    Create a custom algorithm for choosing a best match, it seems wglChoosePixelFormatARB ignores some attributes.
    Seems to completely ignore WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB for example */
    int pixelFormats[64] = { -1 };
    UINT numFormats = 0;
    wglChoosePixelFormatARB(dc, attributes.data(), nullptr, 64, pixelFormats, &numFormats);
    return pixelFormats[0];
}

bool WGLState::IsValid()
{
    return plat_.context != nullptr;
}

void WGLState::CreateContext(DeviceCreateInfo const& createInfo)
{
    glModule_ = LoadLibrary("opengl32.dll");
#define declareWGL(a, b)                                                                          \
    if (b == nullptr) {                                                                           \
        *(reinterpret_cast<void**>(&b)) = reinterpret_cast<void*>(GetProcAddress(glModule_, #b)); \
    }
    WGLFUNCS
#undef declareWGL
    auto backendConfig = static_cast<const BackendExtraGL*>(createInfo.backendConfiguration);

    HGLRC sharedContext = nullptr;
    if (backendConfig) {
        plat_.mhWnd = backendConfig->window;
        sharedContext = backendConfig->sharedContext;
    }

    DoDummy();

    if (!plat_.mhWnd) {
        plat_.mhWnd = GetActiveWindow();
    }
    plat_.display = GetDC(plat_.mhWnd);
    if (wglGetExtensionsStringARB) {
        extensions_ = wglGetExtensionsStringARB(plat_.display);
    }

    PLUGIN_LOG_V("WGL_EXTENSIONS: %s", extensions_.c_str());

    ParseExtensions(extensions_, extensionList_);

    hasSRGBFB_ = HasExtension("WGL_ARB_framebuffer_sRGB");
    hasColorSpace_ = HasExtension("WGL_EXT_colorspace");

    // construct attribute list dynamically
    vector<int> attributes;
    const size_t ATTRIBUTE_RESERVE = 20;       // reserve 20 attributes
    attributes.reserve(ATTRIBUTE_RESERVE * 2); // 2 EGLints per attribute
    const auto addAttribute = [&attributes](int a, int b) {
        attributes.push_back(a);
        attributes.push_back(b);
    };
    addAttribute(WGL_STEREO_ARB, GL_FALSE);
    addAttribute(WGL_AUX_BUFFERS_ARB, 0);
    addAttribute(WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_EXT);
    addAttribute(WGL_DRAW_TO_WINDOW_ARB, GL_TRUE);
    addAttribute(WGL_SUPPORT_OPENGL_ARB, GL_TRUE);
    addAttribute(WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB);
    addAttribute(WGL_DOUBLE_BUFFER_ARB, GL_TRUE);
    addAttribute(WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB);
    addAttribute(WGL_RED_BITS_ARB, 8);
    addAttribute(WGL_GREEN_BITS_ARB, 8);
    addAttribute(WGL_BLUE_BITS_ARB, 8);
    if (hasSRGBFB_) {
        addAttribute(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE); // srgb capable framebuffers only..
    }
    if (hasColorSpace_) {
        addAttribute(WGL_COLORSPACE_EXT, WGL_COLORSPACE_SRGB_EXT); // prefer srgb..
    }
    if (backendConfig) {
        if (backendConfig->MSAASamples > 1) {
            addAttribute(WGL_SAMPLE_BUFFERS_ARB, 1);
            addAttribute(WGL_SAMPLES_ARB, static_cast<int>(backendConfig->MSAASamples));
        }
        addAttribute(WGL_DEPTH_BITS_ARB, static_cast<int>(backendConfig->depthBits));
        addAttribute(WGL_STENCIL_BITS_ARB, static_cast<int>(backendConfig->stencilBits));
        addAttribute(WGL_ALPHA_BITS_ARB, static_cast<int>(backendConfig->alphaBits));
    }
    addAttribute(0, 0); // Terminate the list
    const int pixelFormat = ChoosePixelFormat(plat_.display, attributes);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    DescribePixelFormat(plat_.display, pixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(plat_.display, pixelFormat, &pfd);

    plat_.context = wglCreateContextAttribsARB(plat_.display, sharedContext, WGL_ATTRIBS);

    SaveContext();
    SetContext(nullptr); // activate the context with the dummy PBuffer.
}

void WGLState::DestroyContext()
{
    wglMakeCurrent(nullptr, nullptr);
    if (plat_.context) {
        wglDeleteContext(plat_.context);
    }
    if (plat_.mhWnd && plat_.display) {
        ReleaseDC(plat_.mhWnd, plat_.display);
    }
    FreeLibrary(glModule_);
}

void WGLState::GlInitialize()
{
#define declare(a, b)                                                                             \
    if (b == nullptr) {                                                                           \
        *(reinterpret_cast<void**>(&b)) = reinterpret_cast<void*>(wglGetProcAddress(#b));         \
    }                                                                                             \
    if (b == nullptr) {                                                                           \
        *(reinterpret_cast<void**>(&b)) = reinterpret_cast<void*>(GetProcAddress(glModule_, #b)); \
    }                                                                                             \
    PLUGIN_ASSERT_MSG(b, "Missing %s\n", #b)

#include "gles/gl_functions.h"
#undef declare

    plat_.deviceName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    plat_.driverVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    BASE_NS::ClearToValue(
        &plat_.deviceProperties, sizeof(plat_.deviceProperties), 0x00, sizeof(plat_.deviceProperties));
    FillProperties(plat_.deviceProperties);

    SetSwapInterval(1); // default to vsync enabled.

    glEnable(GL_FRAMEBUFFER_SRGB);
}

void WGLState::SetSwapInterval(uint32_t aInterval)
{
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(static_cast<int>(aInterval));
    }
}

void* WGLState::ErrorFilter() const
{
    return reinterpret_cast<void*>(FilterError);
}

void WGLState::SaveContext()
{
    PLUGIN_ASSERT(!oldIsSet_);
    oldIsSet_ = true;
    oldContext_.context = wglGetCurrentContext();
    oldContext_.display = wglGetCurrentDC();
}

void WGLState::SetContext(const SwapchainGLES* swapChain)
{
    if (swapChain == nullptr) {
        wglMakeCurrent(plat_.display, plat_.context);
    } else {
        const auto& plat = swapChain->GetPlatformData();
        if (plat.surface == 0) {
            const uint64_t swapLoc = reinterpret_cast<uint64_t>(swapChain);
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_ONCE_E(
                "gl_invalid_surface" + BASE_NS::to_string(swapLoc),
                    "Invalid swapchain surface for MakeCurrent using default");
#endif
            wglMakeCurrent(plat_.display, plat_.context);
        } else {
            auto display = reinterpret_cast<HDC>(plat.surface);
            auto context = plat_.context;
            if (plat_.context == 0) {
                PLUGIN_LOG_E("Invalid context for MakeCurrent");
            }
            wglMakeCurrent(display, context);
        }

        if (vSync_ != plat.vsync) {
            vSync_ = plat.vsync;
            SetSwapInterval(plat.vsync ? 1u : 0u);
        }
    }
}

void WGLState::RestoreContext()
{
    PLUGIN_ASSERT(oldIsSet_);
    wglMakeCurrent(oldContext_.display, oldContext_.context);
    oldIsSet_ = false;
}

const DevicePlatformData& WGLState::GetPlatformData() const
{
    return plat_;
}

} // namespace WGLHelpers
RENDER_END_NAMESPACE()
#endif