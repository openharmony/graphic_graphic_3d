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

#if RENDER_HAS_GL_BACKEND
#include "gles/wgl_state.h"

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
#define declareWGL(a, b)                                                                  \
    if (b == nullptr) {                                                                   \
        *(reinterpret_cast<void**>(&b)) = reinterpret_cast<void*>(wglGetProcAddress(#b)); \
    };
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
} // namespace

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
    const int32_t configId = GetPixelFormat(surface);
    return GetInformation(surface, configId, res);
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

    auto other = wglGetCurrentContext();
    DoDummy();
    plat_.mhWnd = GetActiveWindow();
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
    auto backendConfig = static_cast<const BackendExtraGL*>(createInfo.backendConfiguration);
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
#if defined(CORE_CREATE_GLES_CONTEXT_WITH_WGL) && (CORE_CREATE_GLES_CONTEXT_WITH_WGL == 1)
    int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        3,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        2,
#if RENDER_GL_DEBUG
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_ES2_PROFILE_BIT_EXT,
        0
    };
#else
    constexpr int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        4,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        5,
#if RENDER_GL_DEBUG
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
#endif
    plat_.context = wglCreateContextAttribsARB(plat_.display, other, attribs);
    dummyContext_.display = plat_.display;
    dummyContext_.context = plat_.context;

    SaveContext();
    SetContext(nullptr); // activate the context with the dummy PBuffer.
} // namespace WGLHelpers

void WGLState::DestroyContext()
{
    wglMakeCurrent(nullptr, nullptr);
    if (plat_.context != nullptr) {
        wglDeleteContext(plat_.context);
    }
    if ((plat_.mhWnd != nullptr) && (plat_.display != nullptr)) {
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

void WGLState::SetContext(SwapchainGLES* swapChain)
{
    if (swapChain == nullptr) {
        wglMakeCurrent(dummyContext_.display, dummyContext_.context);
    } else {
        ContextState newContext;
        const auto& plat = swapChain->GetPlatformData();
        newContext.display = plat_.display;
        newContext.context = plat_.context;

        wglMakeCurrent(newContext.display, newContext.context);

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