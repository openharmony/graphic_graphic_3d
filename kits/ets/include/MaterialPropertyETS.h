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

#ifndef OHOS_3D_MATERIAL_PROPERTY_ETS_H
#define OHOS_3D_MATERIAL_PROPERTY_ETS_H

#include <scene/interface/intf_texture.h>

#include "ImageETS.h"
#include "Vec4Proxy.h"

namespace OHOS::Render3D {
class MaterialPropertyETS {
public:
    MaterialPropertyETS(const SCENE_NS::ITexture::Ptr tex);
    ~MaterialPropertyETS();

    std::shared_ptr<ImageETS> GetImage();
    void SetImage(const std::shared_ptr<ImageETS> img);

    std::shared_ptr<Vec4Proxy> GetFactor();
    void SetFactor(const BASE_NS::Math::Vec4 &factor);

    SCENE_NS::ITexture::Ptr GetNativeTexture()
    {
        return tex_;
    }

private:
    SCENE_NS::ITexture::Ptr tex_;
    std::shared_ptr<Vec4Proxy> factorProxy_{nullptr};
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_MATERIAL_PROPERTY_ETS_H