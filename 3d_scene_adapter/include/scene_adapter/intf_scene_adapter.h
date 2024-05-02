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

#ifndef OHOS_RENDER_3D_INTF_SCENE_ADAPTER_H
#define OHOS_RENDER_3D_INTF_SCENE_ADAPTER_H

#include <memory>

#include "texture_info.h"

namespace OHOS::Render3D {

class TextureLayer;

class ISceneAdapter {
public:
    virtual bool LoadPluginsAndInit() = 0;
    virtual std::shared_ptr<TextureLayer> CreateTextureLayer() = 0;
    virtual void OnWindowChange(const WindowChangeInfo& windowChangeInfo) = 0;
    virtual void RenderFrame() = 0;
    virtual void Deinit() = 0;
    virtual ~ISceneAdapter() = default;
};

} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_INTF_SCENE_ADAPTER_H
