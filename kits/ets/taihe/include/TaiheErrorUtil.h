/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <cstdarg>
#include <string>
#include <napi_error_util.h>
#include "taihe/runtime.hpp"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D::KITETS {

using ErrorCode = NapiErrorUtil::Code;

FORMAT_FUNC(2, 3)
inline void SetBusinessError(ErrorCode code, const char* fmt, ...)
{
    if (!fmt) {
        WIDGET_LOGW("SetBusinessError: fmt is null");
        taihe::set_business_error(static_cast<int32_t>(code), NapiErrorUtil::ToString(code));
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    std::string buf = NapiErrorUtil::FormatStringV(fmt, ap);
    va_end(ap);
    taihe::set_business_error(static_cast<int32_t>(code), buf.c_str());
}

} // namespace OHOS::Render3D::KITETS
#endif // TAIHE_ERROR_UTIL_H
