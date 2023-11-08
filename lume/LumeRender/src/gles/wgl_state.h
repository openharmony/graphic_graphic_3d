/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
    void SetContext(SwapchainGLES* swapChain);
    void RestoreContext();
    void* ErrorFilter() const;

    bool HasExtension(BASE_NS::string_view) const;

    bool GetSurfaceInformation(HDC surface, GlesImplementation::SurfaceInfo& res) const;

protected:
    DevicePlatformDataGL plat_;
    struct ContextState {
        HGLRC context { nullptr };
        HDC display { nullptr };
    };
    ContextState oldContext_;
    ContextState dummyContext_;
    bool vSync_ = false;
    bool oldIsSet_ = false;
    HMODULE glModule_ = NULL;
    BASE_NS::string extensions_;
    BASE_NS::vector<BASE_NS::string_view> extensionList_;
    bool GetInformation(HDC surface, int32_t configId, GlesImplementation::SurfaceInfo& res) const;
    int ChoosePixelFormat(HDC dc, const BASE_NS::vector<int>& inattributes);
    bool hasSRGBFB_, hasColorSpace_;
};
} // namespace WGLHelpers
RENDER_END_NAMESPACE()

#endif
#endif // GLES_WGL_STATE_H