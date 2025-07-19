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

#ifndef OHOS_3D_BLEND_IMPL_H
#define OHOS_3D_BLEND_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

class BlendImpl {
public:
    BlendImpl()
    {
        // Don't forget to implement the constructor.
    }

    bool getEnabled()
    {
        TH_THROW(std::runtime_error, "getEnabled not implemented");
    }

    void setEnabled(bool enable)
    {
        TH_THROW(std::runtime_error, "setEnabled not implemented");
    }
};
#endif // OHOS_3D_BLEND_IMPL_H