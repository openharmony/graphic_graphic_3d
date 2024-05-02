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

#ifndef GLES_EGL_STATE_H
#define GLES_EGL_STATE_H

#if RENDER_HAS_GLES_BACKEND
#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class SwapchainGLES;
struct DeviceCreateInfo;
struct DevicePlatformData;

namespace GlesImplementation {
struct SurfaceInfo;
} // namespace GlesImplementation

namespace EGLHelpers {
class EGLState {
public:
    EGLState() = default;
    ~EGLState() = default;
    bool CreateContext(DeviceCreateInfo const& createInfo);
    bool IsValid();

    void GlInitialize();
    void DestroyContext();
    void SetSwapInterval(uint32_t interval);
    const DevicePlatformData& GetPlatformData() const;

    void SaveContext();
    void SetContext(const SwapchainGLES* swapchain);
    void RestoreContext();
    void* ErrorFilter() const;

    uint32_t MajorVersion() const;
    uint32_t MinorVersion() const;

    bool HasExtension(BASE_NS::string_view) const;
    uintptr_t CreateSurface(uintptr_t window, uintptr_t instance) const noexcept;
    void DestroySurface(uintptr_t surface) const noexcept;
    bool GetSurfaceInformation(
        const DevicePlatformDataGLES& plat, EGLSurface surface, GlesImplementation::SurfaceInfo& res) const;
    void SwapBuffers(const SwapchainGLES& swapchain);

protected:
    bool VerifyVersion();
    bool hasColorSpaceExt_ { false };
    bool hasConfiglessExt_ { false };
    bool hasSurfacelessExt_ { false };
    void HandleExtensions();
    BASE_NS::string cextensions_;                          // list of client extensions (null terminated strings)
    BASE_NS::vector<BASE_NS::string_view> cextensionList_; // pointers to cextensions_
    BASE_NS::string dextensions_;                          // list of display extensions (null terminated strings)
    BASE_NS::vector<BASE_NS::string_view> dextensionList_; // pointers to dextensions_

    DevicePlatformDataGLES plat_;
    struct ContextState {
        EGLContext context = EGL_NO_CONTEXT;
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLSurface readSurface = EGL_NO_SURFACE;
        EGLSurface drawSurface = EGL_NO_SURFACE;
    };
    ContextState oldContext_;

    EGLSurface dummySurface_ = EGL_NO_SURFACE;
    ContextState dummyContext_;
    bool vSync_ = true;
    bool oldIsSet_ = false;

    void GetContext(ContextState& oldState);
    void SetContext(const ContextState& newState, bool force = false);
    void ChooseConfiguration(const BackendExtraGLES*);
    void CreateContext(const BackendExtraGLES*);
    bool IsVersionGreaterOrEqual(uint32_t major, uint32_t minor) const;
};
} // namespace EGLHelpers
RENDER_END_NAMESPACE()
#endif
#endif // GLES_EGL_STATE_H
