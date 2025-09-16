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

#ifndef OHOS_3D_SCENE_RESOURCE_IMPL_H
#define OHOS_3D_SCENE_RESOURCE_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "SceneResources.proj.hpp"
#include "SceneResources.impl.hpp"

#include "ANIUtils.h"
#include "SceneResourceETS.h"

namespace OHOS::Render3D::KITETS {
class SceneResourceImpl {
public:
    SceneResourceImpl(const SceneResources::SceneResourceType &type, const std::shared_ptr<SceneResourceETS> srETS)
        : type_(type), sceneResourceETS_(srETS)
    {}

    ~SceneResourceImpl()
    {
        sceneResourceETS_.reset();
    }

    ::taihe::string getName();

    void setName(::taihe::string_view name);

    ::SceneResources::SceneResourceType getResourceType();

    ::taihe::optional<uintptr_t> getUri();

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

    void destroy();

private:
    SceneResources::SceneResourceType type_;
    std::shared_ptr<SceneResourceETS> sceneResourceETS_{nullptr};
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SCENE_RESOURCE_IMPL_H