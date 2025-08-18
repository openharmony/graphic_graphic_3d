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

#include "ANIUtils.h"

namespace OHOS::Render3D::KITETS {
std::string ResourceToString(ani_object ani_obj, ani_env *env)
{
    std::string resourceStr;
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_object params;
    env->Object_GetPropertyByName_Ref(ani_obj, "params", reinterpret_cast<ani_ref *>(&params));
    ani_double length;
    env->Object_GetPropertyByName_Double(params, "length", &length);
    WIDGET_LOGD("resource params length %{public}f", length);
    for (int i = 0; i < static_cast<int>(length); i++) {
        ani_ref stringEntryRef;
        env->Object_CallMethodByName_Ref(
            params, "$_get", "I:Lstd/core/Object;", &stringEntryRef, static_cast<ani_int>(i));
        resourceStr = ToStdString(reinterpret_cast<ani_string>(stringEntryRef), env);
        WIDGET_LOGD("string in params: %{public}s", resourceStr.c_str());
        // there supposed to be one resource string
        return resourceStr;
    }
    WIDGET_LOGE("no valid string in resource param");
    return resourceStr;
}

std::string ExtractUri(::taihe::optional_view<uintptr_t> uri, ani_env *env)
{
    std::string uriStr;
    if (!uri) {
        WIDGET_LOGI("uri is empty");
        return uriStr;
    }

    if (env == nullptr) {
        env = taihe::get_env();
    }
    if (IsString(*uri, env)) {
        // actually not supported anymore.
        uriStr = ToStdString(reinterpret_cast<ani_string>(*uri), env);
        WIDGET_LOGD("uri is string, %{public}s", uriStr.c_str());
        // check if there is a protocol
        auto t = uriStr.find("://");
        if (t == std::string::npos) {
            // no proto . so use default
            uriStr.insert(0, "file://");
        }
        return uriStr;
    }

    WIDGET_LOGD("uri is resource");
    // rawfile
    uriStr = ResourceToString(reinterpret_cast<ani_object>(*uri), env);
    if (!uriStr.empty()) {
        // add the schema then
        uriStr.insert(0, "OhosRawFile://");
    }
    return uriStr;
}

SceneETS::RenderParameters ExtractRenderParameters(::taihe::optional_view<::SceneTH::RenderParameters> params)
{
    SceneETS::RenderParameters renderParams;
    if (!params) {
        return renderParams;
    }
    if (params->alwaysRender) {
        renderParams.alwaysRender_ = params->alwaysRender.value();
        renderParams.valid_ = true;
    }
    return renderParams;
}
}  // namespace OHOS::Render3D::KITETS