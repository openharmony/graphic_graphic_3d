/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <string>
#include <vector>

#include "SceneJS.h"
#include "scene_adapter/scene_bridge.h"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
std::shared_ptr<ISceneAdapter> SceneBridge::UnwrapSceneFromJs(napi_env env, napi_value value)
{
    SceneJS* sceneNapi = nullptr;
    napi_status status = napi_unwrap(env, value, reinterpret_cast<void**>(&sceneNapi));

    if (status != napi_ok) {
        WIDGET_LOGE("unwrap scene napi from js failed");
        return nullptr;
    }

    if (sceneNapi == nullptr) {
        WIDGET_LOGE("get scene napi pointer nullptr");
        return nullptr;
    }

    if (!sceneNapi->scene_) {
        WIDGET_LOGE("sceneNapi->scene_ is nullptr");
    }

    return sceneNapi->scene_;
}
} // namespace OHOS::Render3D
