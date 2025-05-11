/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_INTF_SURFACE_BUFFER_INFO_H
#define OHOS_RENDER_3D_INTF_SURFACE_BUFFER_INFO_H

#include <functional>
#include <vector>

#include <refbase.h>
#include <surface_buffer.h>
#include <sync_fence.h>

namespace OHOS::Render3D {
struct SurfaceBufferInfo {
    typedef std::function<void(int32_t)> CallbackFun;
    OHOS::sptr<OHOS::SurfaceBuffer> sfBuffer_ = nullptr;
    OHOS::sptr<OHOS::SyncFence> acquireFence_ = OHOS::SyncFence::INVALID_FENCE;
    std::vector<float> transformMatrix_;
    bool needsTrans_ = false;
    CallbackFun fn_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_RENDER_3D_INTF_SURFACE_BUFFER_INFO_H