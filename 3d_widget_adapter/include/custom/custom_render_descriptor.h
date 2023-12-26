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
