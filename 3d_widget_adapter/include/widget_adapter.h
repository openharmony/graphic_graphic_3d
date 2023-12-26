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

#ifndef OHOS_RENDER_3D_WIDGET_ADAPTER_H
#define OHOS_RENDER_3D_WIDGET_ADAPTER_H

#include <memory>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include "i_engine.h"
#include "data_type/constants.h"
#include "data_type/geometry/geometry.h"
#include "data_type/geometry/cube.h"
#include "data_type/geometry/sphere.h"
#include "data_type/geometry/cone.h"
#include "data_type/gltf_animation.h"
#include "data_type/pointer_event.h"

namespace OHOS::Render3D {

class __attribute__((visibility("default"))) WidgetAdapter {
public:
    explicit WidgetAdapter(uint32_t key);
    virtual ~WidgetAdapter();
    void DeInitEngine();
    WidgetAdapter(const WidgetAdapter&) = delete;
    WidgetAdapter& operator=(const WidgetAdapter&) = delete;

    bool Initialize(std::unique_ptr<IEngine> engine);
    bool OnWindowChange(const TextureInfo& textureInfo);

    bool SetupCameraTransform(const OHOS::Render3D::Position& position, const OHOS::Render3D::Vec3& lookAt,
        const OHOS::Render3D::Vec3& up, const OHOS::Render3D::Quaternion& rotation);
    bool SetupCameraViewProjection(float zNear, float zFar, float fovDegrees);
    bool SetupCameraViewport(uint32_t width, uint32_t height);

    bool OnTouchEvent(const PointerEvent& event);
    bool NeedsRepaint();

    bool DrawFrame();
    bool UpdateGeometries(const std::vector<std::shared_ptr<Geometry>>& shapes);
    bool UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>>& animations);

    bool UpdateLights(const std::vector<std::shared_ptr<OHOS::Render3D::Light>>& lights);
    bool UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor>& customRender);
    bool UpdateShaderPath(const std::string& shaderPath);
    bool UpdateImageTexturePaths(const std::vector<std::string>& imageTextures);
    bool UpdateShaderInputBuffer(const std::shared_ptr<OHOS::Render3D::ShaderInputBuffer>& shaderInputBuffer);

    bool LoadSceneModel(const std::string& scene);
    bool LoadEnvModel(const std::string& enviroment, BackgroundType type);
    bool UnloadSceneModel();
    bool UnloadEnvModel();

private:
    std::unique_ptr<IEngine> engine_ = nullptr;
    uint32_t key_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_WIDGET_ADAPTER_H
