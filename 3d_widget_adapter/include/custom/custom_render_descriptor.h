/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_CUSTOM_DESCRIPTOR_H
#define OHOS_RENDER_3D_CUSTOM_DESCRIPTOR_H

#include <string>

namespace OHOS::Render3D {
struct CustomRenderDescriptor {
public:
    CustomRenderDescriptor(const std::string& rngUri, bool needsFrameCallback) : rngUri_(rngUri),
        needsFrameCallback_(needsFrameCallback) {};
    ~CustomRenderDescriptor() = default;

    std::string GetUri()
    {
        return rngUri_;
    }

    bool NeedsFrameCallback()
    {
        return needsFrameCallback_;
    }

private:
    std::string rngUri_;
    bool needsFrameCallback_ = false;
};
} // namespace name

#endif //OHOS_RENDER_3D_CUSTOM_DESCRIPTOR_H
