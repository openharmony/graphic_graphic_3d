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

#ifndef OHOS_3D_ANI_UTILS_H
#define OHOS_3D_ANI_UTILS_H
#include <algorithm>

#include "3d_widget_adapter_log.h"
#include <ani_signature_builder.h>
#include <base/containers/vector.h>
#include <base/containers/string.h>

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"
#include "taihe/array.hpp"

#include "SceneETS.h"

namespace OHOS::Render3D::KITETS {
/**
 * Determine whether the actual type of ResourceStr is String
 */
inline bool IsString(uintptr_t resourceStr, ani_env *env = nullptr)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_boolean isStr;
    ani_class cls;
    auto stringClass = arkts::ani_signature::Builder::BuildClass({"std", "core", "String"});
    env->FindClass(stringClass.Descriptor().c_str(), &cls);
    env->Object_InstanceOf((ani_object)resourceStr, cls, &isStr);
    return isStr;
}

inline std::string ToStdString(const ani_string &ani_str, ani_env *env = nullptr)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_size sz{};
    env->String_GetUTF8Size(ani_str, &sz);

    std::string result(sz + 1, 0);
    env->String_GetUTF8(ani_str, result.data(), result.size(), &sz);
    result.resize(sz);
    return result;
}

inline ani_string ToANIString(std::string str, ani_env *env = nullptr)
{
    if (env == nullptr) {
        env = taihe::get_env();
    }
    ani_string result_string{};
    if (env->String_NewUTF8(str.c_str(), str.size(), &result_string) != ANI_OK) {
        return {};
    }
    return result_string;
}

std::string ResourceToString(ani_object ani_obj, ani_env *env = nullptr);

std::string ExtractUri(::taihe::optional_view<uintptr_t> uri, ani_env *env = nullptr);

SceneETS::RenderParameters ExtractRenderParameters(::taihe::optional_view<::SceneTH::RenderParameters> params);

template <typename F, typename T>
inline BASE_NS::vector<T> ArrayToVector(const taihe::array<F> &arry, const std::function<T(const F &)> &conv)
{
    BASE_NS::vector<T> items;
    items.resize(arry.size());
    std::transform(arry.begin(), arry.end(), items.begin(), conv);
    return items;
}

template <typename F, typename T>
inline BASE_NS::vector<T> ArrayToVector(
    const taihe::optional<taihe::array<F>> &arrayOp, const std::function<T(const F &)> &conv)
{
    if (arrayOp.has_value()) {
        return ArrayToVector<F, T>(arrayOp.value(), conv);
    } else {
        return {};
    }
}

inline BASE_NS::string ToBaseString(const taihe::string &str)
{
    return BASE_NS::string(str.c_str());
}

inline taihe::string ToTaiheString(const BASE_NS::string &str)
{
    return taihe::string(str.c_str());
}

template<typename T>
inline T *GetImplPointer(const ::taihe::optional<int64_t> &implOp)
{
    if (implOp.has_value()) {
        return reinterpret_cast<T *>(implOp.value());
    } else {
        return nullptr;
    }
}
}  // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_ANI_UTILS_H