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

#ifndef OHOS_3D_IMAGE_IMPL_H
#define OHOS_3D_IMAGE_IMPL_H

#include "stdexcept"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"
#include "SceneTH.user.hpp"

#include "ANIUtils.h"
#include "ParamUtils.h"
#include "ImageETS.h"
#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
class ImageImpl : public SceneResourceImpl {
public:
    static SceneResources::Image createImageFromTH(SceneTH::SceneResourceParameters const &params);
    explicit ImageImpl(const std::shared_ptr<ImageETS> imageETS);
    ~ImageImpl();
    void destroy() override;

    int32_t getWidth();
    int32_t getHeight();

    std::shared_ptr<ImageETS> getInternalImage() const
    {
        return imageETS_;
    }

private:
    std::shared_ptr<ImageETS> imageETS_{nullptr};
};
} // namespace OHOS::Render3D::KITETS
#endif // OHOS_3D_IMAGE_IMPL_H