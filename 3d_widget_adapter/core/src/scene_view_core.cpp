/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "scene_view_core.h"

namespace OHOS::Render3D {
SceneViewCore::~SceneViewCore()
{
    engine_.reset(nullptr);
}

SceneViewCore& SceneViewCore::GetInstance()
{
    static SceneViewCore instance;
    return instance;
}
    
void SceneViewCore::CreateEngine(EngineFactory::EngineType type, const IPlatformCreateInfo& info)
{
    engine_ = EngineFactory::CreateEngine(type, info);
}

TextureInfo SceneViewCore::CreateTargetSceneTexture(uint32_t key, int32_t width, int32_t height)
{
    TextureInfo info;
    if (engine_) {
        info = engine_->CreateTargetSceneTexture(key, width, height);
    }
    return info;
}

void SceneViewCore::SetTargetSceneTexture(uint32_t key, const TextureInfo& info)
{
    if (engine_) {
        engine_->SetTargetSceneTexture(key, info);
    }
}

void SceneViewCore::ImportScene(uint32_t key, const std::string& gltfPath)
{
    if (engine_) {
        engine_->ImportScene(key, gltfPath);
    }
}

void SceneViewCore::ImportBackgroundScene(uint32_t key, const std::string& gltfPath)
{
    if (engine_) {
        engine_->ImportBackgroundScene(key, gltfPath);
    }
}

void SceneViewCore::SetUpCamera(uint32_t key, const std::string& cameraName, const CameraInfo& info)
{
    if (engine_) {
        engine_->SetUpCamera(key, cameraName, info);
    }
}

void SceneViewCore::SetUpLight(uint32_t key, const std::string& lightName, const LightInfo& info)
{
    if (engine_) {
        engine_->SetUpLight(key, lightName, info);
    }
}

void SceneViewCore::SetUpAnimation(uint32_t key, const std::string& animationName)
{
    if (engine_) {
        engine_->SetUpAnimation(key, animationName);
    }
}

void SceneViewCore::SetAnimationState(uint32_t key, const std::string& animationName, AnimationState state)
{
    if (engine_) {
        engine_->SetAnimationState(key, animationName, state);
    }
}

void SceneViewCore::RenderFrame(uint32_t key)
{
    if (engine_) {
        engine_->RenderFrame(key);
    }
}
} // namespace OHOS::Render3D