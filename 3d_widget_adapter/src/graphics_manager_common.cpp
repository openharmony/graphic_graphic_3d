/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "graphics_manager_common.h"

#include "3d_widget_adapter_log.h"
#include "base/log/ace_trace.h"
#include "engine_factory.h"
#include "i_engine.h"
#include "platform_data.h"

namespace OHOS::Render3D {
GraphicsManagerCommon::~GraphicsManagerCommon()
{
    // should never be called
}

void GraphicsManagerCommon::Register(int32_t key)
{
    if (viewTextureMap_.find(key) != viewTextureMap_.end()) {
        return;
    }

    viewTextureMap_[key] = 0;

    return;
}

bool GraphicsManagerCommon::LoadEngineLib()
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::LoadEngineLib");
    if (engine_ == nullptr) {
        return false;
    }

    if (engineLoaded_) {
        return true;
    }

    auto success = engine_->LoadEngineLib();
    if (success) {
        engineLoaded_ = true;
    }

    return success;
}

bool GraphicsManagerCommon::InitEngine(EGLContext eglContext, PlatformData data)
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::InitEngine");
    if (engine_ == nullptr) {
        return false;
    }

    if (engineInited_) {
        return true;
    }

    auto success = engine_->InitEngine(eglContext, data);
    if (success) {
        engineInited_ = true;
    }

    return success;
}

void GraphicsManagerCommon::DeInitEngine()
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::DeInitEngine");
    if (engineInited_ && engine_ != nullptr) {
        engine_->DeInitEngine();
        engineInited_ = false;
    }
}

void GraphicsManagerCommon::UnLoadEngineLib()
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::UnLoadEngineLib");
    if (engineLoaded_ && engine_ != nullptr) {
        engine_->UnLoadEngineLib();
        engineLoaded_ = false;
    }
}

std::unique_ptr<IEngine> GraphicsManagerCommon::GetEngine(EngineFactory::EngineType type, EGLContext eglContext)
{
    WIDGET_LOGD("%s", __func__);
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::GetEngine");

    if (engine_ == nullptr) {
        engine_ = EngineFactory::CreateEngine(type);
        WIDGET_LOGD("create proto engine");
        if (!LoadEngineLib()) {
            WIDGET_LOGE("load engine lib fail");
            return nullptr;
        }

        if (!InitEngine(eglContext, GetPlatformData())) {
            WIDGET_LOGE("init engine fail");
            return nullptr;
        }
    }

    auto client = EngineFactory::CreateEngine(type);
    client->Clone(engine_.get());
    return client;
}

TextureInfo GraphicsManagerCommon::CreateRenderTexture(int32_t key, uint32_t width, uint32_t height,
    EGLContext eglContext)
{
    auto it = viewTextureMap_.find(key);
    if (it == viewTextureMap_.end()) {
        WIDGET_LOGE("%s register yourself before require texture", __func__);
        return {};
    }

    if (eglContext == EGL_NO_CONTEXT) {
        WIDGET_LOGE("%s view with no context", __func__);
        return {};
    }

    it->second = offScreenContextHelper_.CreateRenderTarget(eglContext, width, height);

    return TextureInfo { width, height, it->second };
}

void GraphicsManagerCommon::UnRegister(int32_t key)
{
    WIDGET_LOGD("view unregiser %d total %zu", key, viewTextureMap_.size());
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::UnRegister");

    auto it = viewTextureMap_.find(key);
    if (it == viewTextureMap_.end()) {
        WIDGET_LOGE("view unregiser has not regester");
        return;
    }

    offScreenContextHelper_.DestroyRenderTarget(it->second);

    viewTextureMap_.erase(it);

    if (viewTextureMap_.empty()) {
        // Destroy proto engine
        WIDGET_LOGE("view reset proto engine");
        DeInitEngine();
        UnLoadEngineLib();
        engine_.reset();
        offScreenContextHelper_.DestroyOffScreenContext();
    }
}

bool GraphicsManagerCommon::HasMultiEcs()
{
    return viewTextureMap_.size() > 1;
}

#if MULTI_ECS_UPDATE_AT_ONCE
void GraphicsManagerCommon::DrawFrame(void* ecs)
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::DrawFrame");
    ecss_.push_back(ecs);
    WIDGET_LOGD("ACE-3D DrawFrame ecss size %zu", ecss_.size());
}

void GraphicsManagerCommon::PerformDraw()
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::PerformDraw");
    if (engine_ == nullptr) {
        WIDGET_LOGE("ACE-3D PerformDraw but engine is null");
        return;
    }

    WIDGET_LOGD("ACE-3D PerformDraw");
    engine_->DrawMultiEcs(ecss_);
    ecss_.clear();
}

void GraphicsManagerCommon::AttachContext(const OHOS::Ace::WeakPtr<OHOS::Ace::PipelineContext> context)
{
    OHOS::Ace::ACE_SCOPED_TRACE("GraphicsManagerCommon::AttachContext");
    static bool once = false;
    if (once) {
        return;
    }

    auto pipelineContext = context.Upgrade();
    if (!pipelineContext) {
        WIDGET_LOGE("ACE-3D GraphicsManagerCommon::AttachContext() GetContext failed.");
        return;
    }

    once = true;
    pipelineContext->SetGSVsyncCallback([&] {
        // here we could only push sync task to graphic task, if async manner we
        // have no chance to update the rendering future
        PerformDraw();
    });
}
#endif
} // namespace OHOS::Render3D
