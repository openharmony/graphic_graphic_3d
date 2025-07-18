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

#ifndef OHOS_3D_LIGHT_IMPL_H
#define OHOS_3D_LIGHT_IMPL_H

#include "SceneNodes.proj.hpp"
#include "SceneNodes.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "LightETS.h"

#include "NodeImpl.h"

class LightImpl : public NodeImpl {
private:
    std::shared_ptr<LightETS> lightETS_{nullptr};
public:
    explicit LightImpl(const std::shared_ptr<LightETS> lightETS);

    ::SceneNodes::LightType getLightType();

    ::SceneTypes::Color getColor();
    void setColor(::SceneTypes::weak::Color color);

    double getIntensity();
    void setIntensity(double intensity);

    bool getShadowEnabled();
    void setShadowEnabled(bool enabled);

    bool getEnabled();
    void setEnabled(bool enable);
};

class SpotLightImpl : public LightImpl {
public:
    explicit SpotLightImpl(const std::shared_ptr<LightETS> lightETS);
};

class DirectionalLightImpl : public LightImpl {
public:
    explicit DirectionalLightImpl(const std::shared_ptr<LightETS> lightETS);
};

#endif  // OHOS_3D_LIGHT_IMPL_H