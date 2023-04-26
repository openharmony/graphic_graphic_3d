/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "scene_viewer_adapter.h"

#include "3d_widget_adapter_log.h"
#include "base/log/ace_trace.h"
#include "graphics_manager.h"

namespace OHOS::Render3D {
SceneViewerAdapter::SceneViewerAdapter(uint32_t key) : key_(key)
{
    WIDGET_LOGD("%s %d", __func__, __LINE__);
}

SceneViewerAdapter::~SceneViewerAdapter()
{
    WIDGET_LOGD("%s %d", __func__, __LINE__);
}

bool SceneViewerAdapter::SetUpSceneViewer(const TextureInfo &info, std::string src,
    std::string backgroundSrc, SceneViewerBackgroundType bgType)
{
    WIDGET_LOGD("%s %d", __func__, __LINE__);
    OHOS::Ace::ACE_SCOPED_TRACE("SceneViewerAdapter::SetUpSceneViewer");
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->CreateEcs(key_);
    engine_->CreateScene();
    engine_->CreateCamera();
    engine_->CreateLight();
    engine_->LoadBackgroundModel(backgroundSrc, bgType);
    engine_->LoadSceneModel(src);

    engine_->SetUpCustomRenderTarget(info);
    engine_->SetUpCameraViewPort(info.width_, info.height_);
    engine_->UpdateGLTFAnimations({});

    return true;
}

bool SceneViewerAdapter::SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[])
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->SetUpCameraTransform(position, rotationAngle, rotationAxis);
    return true;
}

bool SceneViewerAdapter::SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->SetUpCameraViewProjection(zNear, zFar, fovDegrees);
    return true;
}

bool SceneViewerAdapter::SetLightProperties(int lightType, float color[], float intensity,
    bool shadow, float position[], float rotationAngle, float rotationAxis[])
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }

    engine_->SetLightProperties(lightType, color, intensity, shadow, position, rotationAngle, rotationAxis);
    return true;
}

bool SceneViewerAdapter::CreateLight()
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->CreateLight();
    return true;
}

bool SceneViewerAdapter::SetUpCustomRenderTarget(const TextureInfo &info)
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->SetUpCustomRenderTarget(info);
    return true;
}

bool SceneViewerAdapter::UnLoadModel()
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s %d", __func__, __LINE__);
        return false;
    }
    engine_->UnLoadModel();
    return true;
}

bool SceneViewerAdapter::OnTouchEvent(const SceneViewerTouchEvent& event)
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->OnTouchEvent(event);
    return true;
}

bool SceneViewerAdapter::IsAnimating()
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    return engine_->IsAnimating();
}

bool SceneViewerAdapter::DrawFrame()
{
    bool ret = true;
    OHOS::Ace::ACE_SCOPED_TRACE("SceneViewerAdapter::DrawFrame");
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        ret = false;
        return ret;
    }
    #if MULTI_ECS_UPDATE_AT_ONCE
    engine_->DeferDraw();
    #else
    engine_->DrawFrame();
    #endif
    return ret;
}

bool SceneViewerAdapter::Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime)
{
    OHOS::Ace::ACE_SCOPED_TRACE("SceneViewerAdapter::Tick");
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->Tick(aTotalTime, aDeltaTime);
    return true;
}

bool SceneViewerAdapter::AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes)
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->AddGeometries(shapes);
    return true;
}

bool SceneViewerAdapter::UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>&
    animations)
{
    if (engine_ == nullptr) {
        WIDGET_LOGE("%s engine not init yet %d", __func__, __LINE__);
        return false;
    }
    engine_->UpdateGLTFAnimations(animations);
    return true;
}

bool SceneViewerAdapter::SetEngine(std::unique_ptr<IEngine> engine)
{
    engine_ = std::move(engine);
    return engine_ != nullptr;
}

void SceneViewerAdapter::DeInitEngine()
{
    OHOS::Ace::ACE_SCOPED_TRACE("SceneViewerAdapter::DeInitEngine");
    WIDGET_LOGD("%s %d", __func__, __LINE__);
    if (engine_ == nullptr) {
        return;
    }
    engine_->DeInitEngine();
    engine_.reset();
}
} // namespace OHOS::Render3D
