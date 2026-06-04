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

#ifndef TAIHE_ERROR_UTIL_H
#define TAIHE_ERROR_UTIL_H

#include <cstdio>
#include <string>
#include <utility>
#include "kits/js/include/napi_error_util.h"
#include "taihe/runtime.hpp"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D::KITETS {

using ErrorCode = NapiErrorUtil::Code;

template<typename... Args>
inline void SetBusinessError(ErrorCode code, const char* fmt, Args&&... args)
{
    if (!fmt) {
        WIDGET_LOGW("SetBusinessError: fmt is null");
        taihe::set_business_error(static_cast<int32_t>(code), nullptr);
        return;
    }
    int size = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (size < 0) return;

    std::string buf(size + 1, '\0');
    std::snprintf(buf.data(), buf.size(), fmt, std::forward<Args>(args)...);
    taihe::set_business_error(static_cast<int32_t>(code), buf.c_str());
}

} // namespace OHOS::Render3D::KITETS
#endif // TAIHE_ERROR_UTIL_H
