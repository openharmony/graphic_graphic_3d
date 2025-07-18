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

#include "taihe/runtime.hpp"
#include "ScenePostProcessSettings.ani.hpp"
#include "SceneTH.ani.hpp"
#include "SceneResources.ani.hpp"
#include "SceneTypes.ani.hpp"
#include "SceneNodes.ani.hpp"

#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    WIDGET_LOGI("Scene ANI_Constructor");
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        WIDGET_LOGE("vm->GetEnv error");
        return ANI_ERROR;
    }
    if (ANI_OK != ScenePostProcessSettings::ANIRegister(env)) {
        WIDGET_LOGE("Error from ScenePostProcessSettings::ANIRegister");
        return ANI_ERROR;
    }
    if (ANI_OK != SceneTH::ANIRegister(env)) {
        WIDGET_LOGE("Error from SceneTH::ANIRegister");
        return ANI_ERROR;
    }
    if (ANI_OK != SceneResources::ANIRegister(env)) {
        WIDGET_LOGE("Error from SceneResources::ANIRegister");
        return ANI_ERROR;
    }
    if (ANI_OK != SceneTypes::ANIRegister(env)) {
        WIDGET_LOGE("Error from SceneTypes::ANIRegister");
        return ANI_ERROR;
    }
    if (ANI_OK != SceneNodes::ANIRegister(env)) {
        WIDGET_LOGE("Error from SceneNodes::ANIRegister");
        return ANI_ERROR;
    }
    *result = ANI_VERSION_1;

    WIDGET_LOGI("Scene ANI_Constructor OK");
    auto sceneAdapter_ = std::make_shared<OHOS::Render3D::SceneAdapter>();
    sceneAdapter_->LoadPluginsAndInit();

    return ANI_OK;
}
