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

#include "SceneResourceETS.h"
#include "Vec4Proxy.h"

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

    EnvironmentETS(const std::string &name, const std::string &uri, SCENE_NS::IEnvironment::Ptr environment);

    ~EnvironmentETS() override;

    META_NS::IObject::Ptr GetNativeObj() const override;

    EnvironmentBackgroundType GetBackgroundType();
    void SetBackgroundType(EnvironmentBackgroundType typeE);

    std::shared_ptr<Vec4Proxy> GetIndirectDiffuseFactor();
    void SetIndirectDiffuseFactor(const BASE_NS::Math::Vec4& factor);
    std::shared_ptr<Vec4Proxy> GetIndirectSpecularFactor();
    void SetIndirectSpecularFactor(const BASE_NS::Math::Vec4& factor);

private:
    SCENE_NS::IEnvironment::Ptr environment_{nullptr};
    std::shared_ptr<Vec4Proxy> diffuseFactor_{nullptr};
    std::shared_ptr<Vec4Proxy> specularFactor_{nullptr};
};
#endif  // OHOS_3D_ENVIRONMENT_ETS_H
