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

#include <napi_api.h>

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid_util.h>
#include <core/log.h>
#include <core/os/platform_create_info.h>

#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include <scene_plugin/interface/intf_scene.h>

#include "CameraJS.h"
#include "EnvironmentJS.h"
#include "GeometryJS.h"
#include "ImageJS.h"
#include "LightJS.h"
#include "MaterialJS.h"
#include "MeshJS.h"
#include "NodeJS.h"
#include "PostProcJS.h"
#include "SceneJS.h"
#include "ShaderJS.h"
#include "SubMeshJS.h"
#include "ToneMapJS.h"
#include "AnimationJS.h"

#include "scene_adapter/scene_adapter.h"
#include "3d_widget_adapter_log.h"

void RegisterClasses(napi_env env, napi_value exports);

namespace OHOS::Render3D {
/*
 * Function registering all props and functions of ohos.medialibrary module
 */
static napi_value Export(napi_env env, napi_value exports)
{
    auto sceneAdapter_ = std::make_shared<OHOS::Render3D::SceneAdapter>();
    sceneAdapter_->LoadPluginsAndInit();

    napi_value Storage;
    napi_create_object(env, &Storage);

    NapiApi::MyInstanceState* mis = new NapiApi::MyInstanceState(env, Storage);

    auto status = napi_set_instance_data(
        env,
        mis,
        [](napi_env env, void *finalize_data, void *finalize_hint) {
            auto d = static_cast<NapiApi::MyInstanceState *>(finalize_data);
            delete d;
        },
        nullptr);

    RegisterClasses(env, exports);

    return exports;
}

/*
 * module define
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "scene.napi",
    .nm_priv = (reinterpret_cast<void *>(0)),
    .reserved = {0}
};

/*
 * module register
 */
extern "C" __attribute__((constructor)) void Graphics3dKitRegisterModule(void)
{
    WIDGET_LOGD("graphics 3d register");
    napi_module_register(&g_module);
}
} // namespace OHOS::Render3D