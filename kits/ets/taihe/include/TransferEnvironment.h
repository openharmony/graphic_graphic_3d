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

#ifndef OHOS_3D_TRANSFER_ENVIRONMENT_H
#define OHOS_3D_TRANSFER_ENVIRONMENT_H

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "nodejstaskqueue.h"

namespace OHOS::Render3D::KITETS {
class TransferEnvironment {
public:
    static bool check(const napi_env &jsenv)
    {
        if (auto taskQueue = GetOrCreateNodeTaskQueue(jsenv); !taskQueue) {
            WIDGET_LOGE("Failed to creating a JS task queue");
            return false;
        }
        // ensure that graphic3d module has loaded
        napi_value result;
        napi_status status = napi_load_module(jsenv, "@ohos.graphics.scene", &result);
        if (status == napi_ok) {
            WIDGET_LOGI("load graphic3d module success.");
            return true;
        } else {
            WIDGET_LOGE("load graphic3d module failed");
            return false;
        }
    }
};
}  // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_TRANSFER_ENVIRONMENT_H