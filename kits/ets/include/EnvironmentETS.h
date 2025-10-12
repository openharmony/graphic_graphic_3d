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

#ifndef OHOS_3D_ENVIRONMENT_ETS_H
#define OHOS_3D_ENVIRONMENT_ETS_H

#include <string>

#include <scene/interface/intf_scene.h>

#include "ImageETS.h"
#include "SceneResourceETS.h"
#include "Vec4Proxy.h"
#include "QuatProxy.h"

namespace OHOS::Render3D {
class EnvironmentETS : public SceneResourceETS {
public:
    enum EnvironmentBackgroundType {
        /**
         * The background is none.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_NONE = 0,

        /**
         * The background is image.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_IMAGE = 1,

        /**
         * The background is cubmap.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_CUBEMAP = 2,

        /**
         * The background is equirectangular.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_EQUIRECTANGULAR = 3,
    };

    EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene);
    EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene, const std::string &name);
    EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene, const std::string &name,
        const std::string &uri);

    SCENE_NS::IScene::Ptr GetScene() const
    {
        return scene_.lock();
    }

    ~EnvironmentETS() override;

    META_NS::IObject::Ptr GetNativeObj() const override;

    EnvironmentBackgroundType GetBackgroundType();
    void SetBackgroundType(EnvironmentBackgroundType typeE);

    std::shared_ptr<Vec4Proxy> GetIndirectDiffuseFactor();
    void SetIndirectDiffuseFactor(const BASE_NS::Math::Vec4 &factor);
    std::shared_ptr<Vec4Proxy> GetIndirectSpecularFactor();
    void SetIndirectSpecularFactor(const BASE_NS::Math::Vec4 &factor);

    std::shared_ptr<Vec4Proxy> GetEnvironmentMapFactor();
    void SetEnvironmentMapFactor(const BASE_NS::Math::Vec4 &factor);

    std::shared_ptr<ImageETS> GetEnvironmentImage();
    void SetEnvironmentImage(const std::shared_ptr<ImageETS> &image);

    std::shared_ptr<ImageETS> GetRadianceImage();
    void SetRadianceImage(const std::shared_ptr<ImageETS> &image);

    BASE_NS::vector<BASE_NS::Math::Vec3> GetIrradianceCoefficients();
    void SetIrradianceCoefficients(const BASE_NS::vector<BASE_NS::Math::Vec3>& coefficients);

    std::shared_ptr<QuatProxy> GetEnvironmentRotation();
    void SetEnvironmentRotation(const BASE_NS::Math::Quat &rotation);

private:
    SCENE_NS::IEnvironment::Ptr environment_{nullptr};
    SCENE_NS::IScene::WeakPtr scene_{nullptr};
    std::shared_ptr<Vec4Proxy> diffuseFactor_{nullptr};
    std::shared_ptr<Vec4Proxy> specularFactor_{nullptr};
    std::shared_ptr<Vec4Proxy> envMapFactor_{nullptr};
    std::shared_ptr<QuatProxy> envRotation_{nullptr};
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_ENVIRONMENT_ETS_H
