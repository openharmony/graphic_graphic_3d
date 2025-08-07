/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "scene_adapter/scene_bridge_ani.h"

#include <string>
#include <vector>

#include <ani_signature_builder.h>

#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {

void* SceneBridgeAni::UnwrapSceneFromAni(ani_env* env, ani_object value)
{
    WIDGET_LOGD("ace_lume UnwrapSceneFromAni");
    if (!env) {
        WIDGET_LOGE("ace_lume UnwrapSceneFromAni env null");
        return {};
    }

    arkts::ani_signature::Type clsSignature = arkts::ani_signature::Builder::BuildClass("graphics3d/Scene/Scene");
    ani_class sceneCls;
    if (env->FindClass(clsSignature.Descriptor().c_str(), &sceneCls) != ANI_OK) {
        WIDGET_LOGE("ace_lume find scene class fail");
        return {};
    }
    arkts::ani_signature::SignatureBuilder sb{};
    ani_method getSceneNativeMethod;
    sb.SetReturnLong();
    if (env->Class_FindMethod(sceneCls, "getSceneNative", sb.BuildSignatureDescriptor().c_str(),
                              &getSceneNativeMethod) != ANI_OK) {
        WIDGET_LOGE("ace_lume find getSceneNative fail");
        return {};
    }
    ani_long nativePtr;
    if (env->Object_CallMethod_Long(value, getSceneNativeMethod, &nativePtr) != ANI_OK) {
        WIDGET_LOGE("ace_lume find getSceneNativeMethod fail");
        return {};
    }

    auto sceneAdapter = reinterpret_cast<void *>(nativePtr);
    if (sceneAdapter == nullptr) {
        WIDGET_LOGE("ace_lume sceneAdapter null");
        return {};
    }
    WIDGET_LOGD("ace_lume get scene success");
    return sceneAdapter;
}
} // namespace OHOS::Render3D