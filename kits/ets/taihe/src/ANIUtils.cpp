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
ani_object WrapDoubleAsObj(const ani_double value, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char *className = "std.core.Double";
    ani_class doubleCls {};
    env->FindClass(className, &doubleCls);
    ani_method ctor {};
    env->Class_FindMethod(doubleCls, "<ctor>", "d:", &ctor);
    ani_object obj {};
    env->Object_New(doubleCls, ctor, &obj, static_cast<ani_double>(value));
    return obj;
}

ani_double ParseObjToDouble(ani_object obj, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_double value = 0.0;
    env->Object_CallMethodByName_Double(obj, "toDouble", ":d", &value);
    return value;
}

ani_object WrapIntAsObj(const ani_int value, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char *className = "std.core.Int";
    ani_class intCls {};
    env->FindClass(className, &intCls);
    ani_method ctor {};
    env->Class_FindMethod(intCls, "<ctor>", "i:", &ctor);
    ani_object obj {};
    env->Object_New(intCls, ctor, &obj, static_cast<ani_int>(value));
    return obj;
}

ani_int ParseObjToInt(ani_object obj, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_int value = 0;
    env->Object_CallMethodByName_Int(obj, "toInt", ":i", &value);
    return value;
}

ani_object WrapBoolAsObj(const ani_boolean value, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    static constexpr const char *className = "std.core.Boolean";
    ani_class boolCls {};
    env->FindClass(className, &boolCls);
    ani_method ctor {};
    env->Class_FindMethod(boolCls, "<ctor>", "z:", &ctor);
    ani_object obj {};
    env->Object_New(boolCls, ctor, &obj, static_cast<ani_boolean>(value));
    return obj;
}

ani_boolean ParseObjToBool(ani_object obj, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_boolean value = ANI_FALSE;
    env->Object_CallMethodByName_Boolean(obj, "toBoolean", ":z", &value);
    return value;
}

ani_object WrapColorAsObj(::SceneTypes::Color color, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    
    ani_object obj {};

    ani_class colorCls {};
    ani_status status = env->FindClass("graphics3d.SceneTypes._taihe_Color_inner", &colorCls);
    if (status != ANI_OK) {
        WIDGET_LOGE("find color class failed, status: %{public}d", status);
        return obj;
    }

    ani_method initMethod {};
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

BASE_NS::Color ParseObjToColor(ani_object obj, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_double r = 0.0;
    ani_double g = 0.0;
    ani_double b = 0.0;
    ani_double a = 0.0;

    env->Object_GetPropertyByName_Double(obj, "r", &r);
    env->Object_GetPropertyByName_Double(obj, "g", &g);
    env->Object_GetPropertyByName_Double(obj, "b", &b);
    env->Object_GetPropertyByName_Double(obj, "a", &a);

    BASE_NS::Color color = BASE_NS::Color(r, g, b, a);
    return color;
}

AniObjectType HandleAniObject(ani_object obj, ani_env *env)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_class intClass {};
    env->FindClass("std.core.Int", &intClass);

    ani_class doubleClass {};
    env->FindClass("std.core.Double", &doubleClass);

    ani_class boolClass {};
    env->FindClass("std.core.Boolean", &boolClass);

    ani_class colorClass {};
    env->FindClass("graphics3d.SceneTypes.Color", &colorClass);

    ani_boolean isInteger = ANI_FALSE;
    env->Object_InstanceOf(obj, intClass, &isInteger);
    if (isInteger) {
        return AniObjectType::TYPE_INT;
    }

    ani_boolean isDouble = ANI_FALSE;
    env->Object_InstanceOf(obj, doubleClass, &isDouble);
    if (isDouble) {
        return AniObjectType::TYPE_DOUBLE;
    }

    ani_boolean isBoolean = ANI_FALSE;
    env->Object_InstanceOf(obj, boolClass, &isBoolean);
    if (isBoolean) {
        return AniObjectType::TYPE_BOOLEAN;
    }

    ani_boolean isColor = ANI_FALSE;
    env->Object_InstanceOf(obj, colorClass, &isColor);
    if (isColor) {
        return AniObjectType::TYPE_COLOR;
    }

    return AniObjectType::TYPE_UNKNOWN;
}

std::string ResourceToString(ani_object ani_obj, ani_env *env)
{
    static std::string GET_METHOD_SIGNATURE = "";
    if (GET_METHOD_SIGNATURE.empty()) {
        arkts::ani_signature::SignatureBuilder builder{};
        builder.AddInt().SetReturnUndefined();
        GET_METHOD_SIGNATURE = builder.BuildSignatureDescriptor();
    }
    std::string resourceStr;
    if (env == nullptr) {
        env = taihe::get_env();
    }

    ani_object params;
    env->Object_GetPropertyByName_Ref(ani_obj, "params", reinterpret_cast<ani_ref *>(&params));
    ani_int length;
    env->Object_GetPropertyByName_Int(params, "length", &length);
    WIDGET_LOGD("resource params length %{public}d", length);
    for (int i = 0; i < static_cast<int>(length); i++) {
        ani_ref stringEntryRef;
        env->Object_CallMethodByName_Ref(
            params, "$_get", GET_METHOD_SIGNATURE.c_str(), &stringEntryRef, static_cast<ani_int>(i));
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