/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_RENDER_3D_TEXTURE_LAYER_H
#define OHOS_RENDER_3D_TEXTURE_LAYER_H

#include <cstdint>

#include <render_service_client/core/ui/rs_node.h>
#include "texture_info.h"

namespace OHOS::Render3D {

class __attribute__((visibility("default"))) TextureLayer {
public:
    explicit TextureLayer(int32_t key);
    virtual ~TextureLayer();

    virtual void DestroyRenderTarget();
    virtual TextureInfo GetTextureInfo();

    virtual void SetParent(std::shared_ptr<Rosen::RSNode>& parent);
    virtual TextureInfo OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
        bool recreateWindow, SurfaceType surfaceType = SurfaceType::SURFACE_WINDOW);
    virtual TextureInfo OnWindowChange(const WindowChangeInfo& windowChangeInfo);

protected:
    TextureLayer() = default;

private:
    std::shared_ptr<TextureLayer> textureLayer_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_TEXTURE_LAYER_H
