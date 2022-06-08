/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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