/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "scene_adapter/scene_bridge_ani_loader.h"

#include <dlfcn.h>

#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
namespace {
const char* UNWRAP_SCENE_FROM_ANI_SYMBOL = "OHOS_Render3D_UnwrapSceneFromAni";
const char* SCENE_BRIDGE_ANI_LIB = "libscene_bridge_ani.z.so";
}

void* SceneBridgeAniLoader::UnwrapSceneFromAni(void* env, void* value)
{
    if (!env) {
        WIDGET_LOGE("ace_lume SceneBridgeAniLoader UnwrapSceneFromAni env null");
        return nullptr;
    }

    if (!unwrapFunc_) {
        WIDGET_LOGE("ace_lume SceneBridgeAniLoader UnwrapSceneFromAni unwrapFunc null");
        return nullptr;
    }

    return unwrapFunc_(env, value);
}

void SceneBridgeAniLoader::CloseLibrary()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
        unwrapFunc_ = nullptr;
    }
}

void* SceneBridgeAniLoader::LoadSymbol(const char* name)
{
    if (!handle_) {
        WIDGET_LOGE("ace_lume SceneBridgeAniLoader LoadSymbol handle null");
        return nullptr;
    }
    return dlsym(handle_, name);
}

bool SceneBridgeAniLoader::DynamicLoadLibrary()
{
    if (!handle_) {
        handle_ = dlopen(SCENE_BRIDGE_ANI_LIB, RTLD_LAZY);
        if (!handle_) {
            WIDGET_LOGE("ace_lume SceneBridgeAniLoader DynamicLoadLibrary failed");
            return false;
        }
        unwrapFunc_ = reinterpret_cast<UnwrapSceneFromAniFunc>(LoadSymbol(UNWRAP_SCENE_FROM_ANI_SYMBOL));
    }
    return true;
}
} // namespace OHOS::Render3D
