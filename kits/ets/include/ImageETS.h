/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_IMAGE_ETS_H
#define OHOS_3D_IMAGE_ETS_H

#include <string>

#include <base/containers/unordered_map.h>

#include "RenderResourcesETS.h"
#include "SceneResourceETS.h"

namespace OHOS::Render3D {
class ImageETS : public SceneResourceETS {
public:
    ImageETS(const std::string &name, const std::string &uri, const SCENE_NS::IBitmap::Ptr bitmap);
    explicit ImageETS(const SCENE_NS::IImage::Ptr &image);
    ~ImageETS() override;

    META_NS::IObject::Ptr GetNativeObj() const override;
    SCENE_NS::IImage::Ptr GetNativeImage() const
    {
        return bitmap_;
    }
    int32_t GetWidth() const;
    int32_t GetHeight() const;

private:
    SCENE_NS::IBitmap::Ptr bitmap_{nullptr};
    std::shared_ptr<RenderResourcesETS> resources_{nullptr};
};
}  // namespace OHOS::Render3D
#endif // OHOS_3D_IMAGE_ETS_H
