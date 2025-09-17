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

#ifndef OHOS_3D_SCENE_COMPONENT_IMPL_H
#define OHOS_3D_SCENE_COMPONENT_IMPL_H

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "SceneResources.user.hpp"
#include "SceneNodes.user.hpp"

#include "SceneComponentETS.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {
class SceneComponentImpl {
public:
    explicit SceneComponentImpl(std::shared_ptr<SceneComponentETS> sceneComponentETS);

    int32_t getPropertySize();
    ::taihe::array<::taihe::string> getPropertyKeys();
    ::taihe::optional<::SceneTH::ComponentPropertyType> getComponentProperty(::taihe::string_view key);
    void setComponentProperty(::taihe::string_view key, ::SceneTH::ComponentPropertyType const& value);

    ::taihe::string getName();
    void setName(::taihe::string_view name);

private:
    void SetPropertyFromSceneResource(const std::string &name, const ::SceneResources::SceneResource &sr);
    void SetArrayPropertyFromSceneResource(const std::string &name,
        const ::taihe::array<::SceneResources::SceneResource> &srArray);
    std::shared_ptr<SceneComponentETS> sceneComponentETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SCENE_COMPONENT_IMPL_H