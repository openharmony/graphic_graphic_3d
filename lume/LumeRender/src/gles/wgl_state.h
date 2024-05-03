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

#ifndef GLES_WGL_STATE_H
#define GLES_WGL_STATE_H

#if _WIN32

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class SwapchainGLES;
struct DeviceCreateInfo;
struct DevicePlatformData;

namespace GlesImplementation {
struct SurfaceInfo;
} // namespace GlesImplementation

namespace WGLHelpers {
class WGLState final {
public:
    WGLState() = default;
    ~WGLState() = default;
    void CreateContext(DeviceCreateInfo const& createInfo);
    bool IsValid();
    void GlInitialize();
    void DestroyContext();
    void SetSwapInterval(uint32_t interval);
    const DevicePlatformData& GetPlatformData() const;

    void SaveContext();
    void SetContext(const SwapchainGLES* swapChain);
    void RestoreContext();
    void* ErrorFilter() const;

    bool HasExtension(BASE_NS::string_view) const;
    uintptr_t CreateSurface(uintptr_t window, uintptr_t instance) const noexcept;
    void DestroySurface(uintptr_t surface) const noexcept;
    bool GetSurfaceInformation(HDC surface, GlesImplementation::SurfaceInfo& res) const;
    void SwapBuffers(const SwapchainGLES& swapChain);

protected:
    bool GetInformation(HDC surface, int32_t configId, GlesImplementation::SurfaceInfo& res) const;
    int ChoosePixelFormat(HDC dc, const BASE_NS::vector<int>& inattributes);

    HMODULE glModule_ = nullptr;
    DevicePlatformDataGL plat_;
    struct ContextState {
        HGLRC context { nullptr };
        HDC display { nullptr };
    };
    ContextState oldContext_;
    ContextState dummyContext_;
    BASE_NS::string extensions_;
    BASE_NS::vector<BASE_NS::string_view> extensionList_;
    bool vSync_ = true;
    bool oldIsSet_ = false;
    bool hasSRGBFB_ = false;
    bool hasColorSpace_ = false;
};
} // namespace WGLHelpers
RENDER_END_NAMESPACE()

#endif
#endif // GLES_WGL_STATE_H