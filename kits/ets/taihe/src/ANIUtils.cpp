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
struct TypeChecker {
    const char* className;
    AniObjectType type;

    bool Check(ani_object obj, ani_env* env) const
    {
        ani_class cls{};
        ani_status status = env->FindClass(className, &cls);
        if (status != ANI_OK) {
            return false;
        }

        ani_boolean isMatch = ANI_FALSE;
        status = env->Object_InstanceOf(obj, cls, &isMatch);
        if (status != ANI_OK) {
            return false;
        }

        return isMatch == ANI_TRUE;
    }
};

static const TypeChecker typeCheckers[] = {
    {"std.core.Int", AniObjectType::TYPE_INT},
    {"std.core.Double", AniObjectType::TYPE_DOUBLE},
    {"std.core.Boolean", AniObjectType::TYPE_BOOLEAN},
    {"graphics3d.SceneTypes.Color", AniObjectType::TYPE_COLOR},
};

ani_object WrapDoubleAsObj(const ani_double value, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char* className = "std.core.Double";
    ani_object obj{};

    ani_class doubleCls{};
    ani_status status = env->FindClass(className, &doubleCls);
    if (status != ANI_OK) {
        WIDGET_LOGE("find double class failed, status: %{public}d", status);
        return obj;
    }

    ani_method ctor{};
    status = env->Class_FindMethod(doubleCls, "<ctor>", "d:", &ctor);
    if (status != ANI_OK) {
        WIDGET_LOGE("find double init method failed, status: %{public}d", status);
        return obj;
    }

    status = env->Object_New(doubleCls, ctor, &obj, static_cast<ani_double>(value));
    if (status != ANI_OK) {
        WIDGET_LOGE("create new ani double failed, status: %{public}d", status);
        return obj;
    }

    return obj;
}

ani_double ParseObjToDouble(ani_object obj, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_double value = 0.0;
    ani_status status = env->Object_CallMethodByName_Double(obj, "toDouble", ":d", &value);
    if (status != ANI_OK) {
        WIDGET_LOGE("parse obj to aniDouble failed, status: %{public}d", status);
    }
    return value;
}

ani_object WrapIntAsObj(const ani_int value, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char* className = "std.core.Int";
    ani_object obj{};

    ani_class intCls{};
    ani_status status = env->FindClass(className, &intCls);
    if (status != ANI_OK) {
        WIDGET_LOGE("find int class failed, status: %{public}d", status);
        return obj;
    }

    ani_method ctor{};
    status = env->Class_FindMethod(intCls, "<ctor>", "i:", &ctor);
    if (status != ANI_OK) {
        WIDGET_LOGE("find int init method failed, status: %{public}d", status);
        return obj;
    }

    status = env->Object_New(intCls, ctor, &obj, static_cast<ani_int>(value));
    if (status != ANI_OK) {
        WIDGET_LOGE("create new ani int failed, status: %{public}d", status);
        return obj;
    }

    return obj;
}

ani_int ParseObjToInt(ani_object obj, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_int value = 0;
    ani_status status = env->Object_CallMethodByName_Int(obj, "toInt", ":i", &value);
    if (status != ANI_OK) {
        WIDGET_LOGE("parse obj to aniInt failed, status: %{public}d", status);
    }

    return value;
}

ani_object WrapBoolAsObj(const ani_boolean value, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char* className = "std.core.Boolean";
    ani_object obj{};

    ani_class boolCls{};
    ani_status status = env->FindClass(className, &boolCls);
    if (status != ANI_OK) {
        WIDGET_LOGE("find bool class failed, status: %{public}d", status);
        return obj;
    }

    ani_method ctor{};
    status = env->Class_FindMethod(boolCls, "<ctor>", "z:", &ctor);
    if (status != ANI_OK) {
        WIDGET_LOGE("find bool init method failed, status: %{public}d", status);
        return obj;
    }

    status = env->Object_New(boolCls, ctor, &obj, static_cast<ani_boolean>(value));
    if (status != ANI_OK) {
        WIDGET_LOGE("create new ani bool failed, status: %{public}d", status);
        return obj;
    }

    return obj;
}

ani_boolean ParseObjToBool(ani_object obj, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_boolean value = ANI_FALSE;
    ani_status status = env->Object_CallMethodByName_Boolean(obj, "toBoolean", ":z", &value);
    if (status != ANI_OK) {
        WIDGET_LOGE("parse obj to aniBool failed, status: %{public}d", status);
    }

    return value;
}

ani_object WrapColorAsObj(::SceneTypes::Color color, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_object obj{};

    ani_class colorCls{};
    ani_status status = env->FindClass("graphics3d.SceneTypes._taihe_Color_inner", &colorCls);
    if (status != ANI_OK) {
        WIDGET_LOGE("find color class failed, status: %{public}d", status);
        return obj;
    }

    ani_method initMethod{};
    status = env->Class_FindMethod(colorCls, "<ctor>", "ll:", &initMethod);
    if (status != ANI_OK) {
        WIDGET_LOGE("find color init method failed, status: %{public}d", status);
        return obj;
    }

    ani_long ani_vtbl_ptr = reinterpret_cast<ani_long>(color.m_handle.vtbl_ptr);
    ani_long ani_data_ptr = reinterpret_cast<ani_long>(color.m_handle.data_ptr);
    color.m_handle.data_ptr = nullptr;

    status = env->Object_New(colorCls, initMethod, &obj, ani_vtbl_ptr, ani_data_ptr);
    if (status != ANI_OK) {
        WIDGET_LOGE("create new ani color failed, status: %{public}d", status);
        return obj;
    }
    return obj;
}

BASE_NS::Color ParseObjToColor(ani_object obj, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_double r = 0.0;
    ani_double g = 0.0;
    ani_double b = 0.0;
    ani_double a = 0.0;

    ani_status rStatus = env->Object_GetPropertyByName_Double(obj, "r", &r);
    ani_status gStatus = env->Object_GetPropertyByName_Double(obj, "g", &g);
    ani_status bStatus = env->Object_GetPropertyByName_Double(obj, "b", &b);
    ani_status aStatus = env->Object_GetPropertyByName_Double(obj, "a", &a);

    if (rStatus != ANI_OK || gStatus != ANI_OK || bStatus != ANI_OK || aStatus != ANI_OK) {
        WIDGET_LOGE("ParseObjToColor failed, rStatus: %{public}d, gStatus: %{public}d, bStatus: %{public}d, aStatus: "
                    "%{public}d",
            rStatus,
            gStatus,
            bStatus,
            aStatus);
        return BASE_NS::Color(0, 0, 0, 0);
    }

    BASE_NS::Color color = BASE_NS::Color(r, g, b, a);
    return color;
}

AniObjectType HandleAniObject(ani_object obj, ani_env* env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }

    for (const auto& checker : typeCheckers) {
        if (checker.Check(obj, env)) {
            return checker.type;
        }
    }
    return AniObjectType::TYPE_UNKNOWN;
}

std::string ResourceToString(ani_object ani_obj, ani_env* env)
{
    std::string resourceStr;
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_object params;
    ani_status status = env->Object_GetPropertyByName_Ref(ani_obj, "params", reinterpret_cast<ani_ref*>(&params));
    if (status != ANI_OK) {
        WIDGET_LOGE("get the params of Resource failed, status: %{public}d", status);
        return resourceStr;
    }

    ani_size len;
    status = env->Array_GetLength(reinterpret_cast<ani_array>(params), &len);
    if (status != ANI_OK) {
        WIDGET_LOGE("get params length failed, status: %{public}d", status);
        return resourceStr;
    }
    if (len <= 0) {
        WIDGET_LOGE("the params of Resource is empty");
        return resourceStr;
    }

    ani_ref param0;
    status = env->Array_Get(reinterpret_cast<ani_array>(params), 0, &param0);
    if (status != ANI_OK) {
        WIDGET_LOGE("get the first element of params failed, status: %{public}d", status);
        return resourceStr;
    }
    resourceStr = ToStdString(reinterpret_cast<ani_string>(param0), env);
    return resourceStr;
}

std::string ExtractUri(uintptr_t uri, ani_env *env)
{
    std::string uriStr;
    if (!uri) {
        return uriStr;
    }

    if (env == nullptr) {
        env = taihe::get_env();
    }
    if (IsString(uri, env)) {
        // actually not supported anymore.
        uriStr = ToStdString(reinterpret_cast<ani_string>(uri), env);
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
    uriStr = ResourceToString(reinterpret_cast<ani_object>(uri), env);
    if (!uriStr.empty()) {
        // add the schema then
        uriStr.insert(0, "OhosRawFile://");
    }
    return uriStr;
}

std::string ExtractUri(::taihe::optional_view<uintptr_t> uri, ani_env *env)
{
    std::string uriStr;
    if (!uri) {
        return uriStr;
    }
    return ExtractUri(uri.value(), env);
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

SceneETS::SceneLoadParams ExtractSceneLoadParams(::SceneTH::SceneLoadParams params)
{
    SceneETS::SceneLoadParams loadParams;
    if (params.offset) {
        loadParams.offset = params.offset.value();
    }
    return loadParams;
}
}  // namespace OHOS::Render3D::KITETS