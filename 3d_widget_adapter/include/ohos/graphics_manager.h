/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_GRAPHICS_MANAGER_H
#define OHOS_RENDER_3D_GRAPHICS_MANAGER_H

#include "graphics_manager_common.h"
#include <GLES/gl.h>
#include <EGL/egl.h>

namespace OHOS::Render3D {
class __attribute__((visibility("default"))) GraphicsManager : public GraphicsManagerCommon {
public:
    static GraphicsManager& GetInstance();
    PlatformData GetPlatformData(const HapInfo& hapInfo) const override;
    PlatformData GetPlatformData() const override;
    bool GetUseBasisEngine()
    {
        return useBasisEngine_; // get useBasisEngine_ of GraphicsManager, which is global singleton
    }
    void SetUseBasisEngine(bool useBasisEngine)
    {
        useBasisEngine_ = useBasisEngine; // set useBasisEngine_ of GraphicsManager, which is global singleton
    }
private:
    GraphicsManager(const GraphicsManager&) = delete;
    GraphicsManager& operator=(const GraphicsManager&) = delete;
    GraphicsManager() = default;
    virtual ~GraphicsManager();
    bool useBasisEngine_ = false; // set to true by ProductBasis if in rain or snow scene, which uses physics engine
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHICS_MANAGER_H
