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

#include "graphics_manager_common.h"

#include "3d_widget_adapter_log.h"
#include "engine_factory.h"
#include "i_engine.h"
#include "platform_data.h"
#include "widget_trace.h"

namespace OHOS::Render3D {
GraphicsManagerCommon::~GraphicsManagerCommon()
{
    // should never be called
}

void GraphicsManagerCommon::Register(int32_t key, RenderBackend backend)
{
    if (viewTextures_.find(key) != viewTextures_.end()) {
        return;
    }

    viewTextures_.insert(key);
    backends_[key] = backend;
    return;
}

bool GraphicsManagerCommon::LoadEngineLib()
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::LoadEngineLib");
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
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::InitEngine");
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
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::DeInitEngine");
    if (engineInited_ && engine_ != nullptr) {
        engine_->DeInitEngine();
        engineInited_ = false;
    }
}

void GraphicsManagerCommon::UnloadEngineLib()
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::UnLoadEngineLib");
    if (engineLoaded_ && engine_ != nullptr) {
        engine_->UnloadEngineLib();
        engineLoaded_ = false;
    }
}

std::unique_ptr<IEngine> GraphicsManagerCommon::GetEngine(EngineFactory::EngineType type, int32_t key,
    const HapInfo& hapInfo)
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::GetEngine");
    auto backend = backends_.find(key);
    if (backend == backends_.end() || backend->second == RenderBackend::UNDEFINE) {
        WIDGET_LOGE("Get engine before register");
        return nullptr;
    }

    if (backend->second != RenderBackend::GLES) {
        WIDGET_LOGE("not support backend yet");
        return nullptr;
    }

    // gles context
    if (engine_ == nullptr) {
        auto context = offScreenContextHelper_.CreateOffScreenContext(EGL_NO_CONTEXT);
        engine_ = EngineFactory::CreateEngine(type);
        WIDGET_LOGD("create proto engine");
        if (!LoadEngineLib()) {
            WIDGET_LOGE("load engine lib fail");
            return nullptr;
        }

        if (!InitEngine(context, GetPlatformData(hapInfo))) {
            WIDGET_LOGE("init engine fail");
            return nullptr;
        }
    }

    hapInfo_ = hapInfo;
    auto client = EngineFactory::CreateEngine(type);
    client->Clone(engine_.get());
    return client;
}

std::unique_ptr<IEngine> GraphicsManagerCommon::GetEngine(EngineFactory::EngineType type, int32_t key)
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::GetEngine");
    if (viewTextures_.size() > 1u) {
        WIDGET_LOGD("view is not unique and view size is %zu", viewTextures_.size());
    }
    auto backend = backends_.find(key);
    if (backend == backends_.end() || backend->second == RenderBackend::UNDEFINE) {
        WIDGET_LOGE("Get engine before register");
        return nullptr;
    }

    if (backend->second != RenderBackend::GLES) {
        WIDGET_LOGE("not support backend yet");
        return nullptr;
    }

    // gles context
    if (engine_ == nullptr) {
        auto context = offScreenContextHelper_.CreateOffScreenContext(EGL_NO_CONTEXT);
        engine_ = EngineFactory::CreateEngine(type);
        WIDGET_LOGD("create proto engine");
        if (!LoadEngineLib()) {
            WIDGET_LOGE("load engine lib fail");
            return nullptr;
        }

        if (!InitEngine(context, GetPlatformData())) {
            WIDGET_LOGE("init engine fail");
            return nullptr;
        } else {
            WIDGET_LOGD("engine is initialized");
        }
    } else {
        WIDGET_LOGD("engine is initialized");
    }

    auto client = EngineFactory::CreateEngine(type);
    client->Clone(engine_.get());
    return client;
}

EGLContext GraphicsManagerCommon::GetOrCreateOffScreenContext(EGLContext eglContext)
{
    AutoRestore scope;
    return offScreenContextHelper_.CreateOffScreenContext(eglContext);
}

void GraphicsManagerCommon::BindOffScreenContext()
{
    offScreenContextHelper_.BindOffScreenContext();
}

void GraphicsManagerCommon::UnRegister(int32_t key)
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::UnRegister");
    WIDGET_LOGD("view unregiser %d total %zu", key, viewTextures_.size());

    auto it = viewTextures_.find(key);
    if (it == viewTextures_.end()) {
        WIDGET_LOGE("view unregiser has not regester");
        return;
    }

    viewTextures_.erase(it);
    auto backend = backends_.find(key);
    if (backend != backends_.end()) {
        backends_.erase(backend);
    }

    if (viewTextures_.empty()) {
        // Destroy proto engine
        WIDGET_LOGE("view reset proto engine");
        DeInitEngine();
        engine_.reset();
        offScreenContextHelper_.DestroyOffScreenContext();
    }
    // need graphics task exit!!!
}

RenderBackend GraphicsManagerCommon::GetRenderBackendType(int32_t key)
{
    RenderBackend backend = RenderBackend::UNDEFINE;
    auto it = backends_.find(key);
    if (it != backends_.end()) {
        backend = it->second;
    }
    return backend;
}

const HapInfo& GraphicsManagerCommon::GetHapInfo() const
{
    return hapInfo_;
}

bool GraphicsManagerCommon::HasMultiEcs()
{
    return viewTextures_.size() > 1;
}

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
void GraphicsManagerCommon::UnloadEcs(void* ecs)
{
    WIDGET_LOGD("ACE-3D GraphicsService::UnloadEcs()");
    ecss_.erase(ecs);
}

void GraphicsManagerCommon::DrawFrame(void* ecs, void* handles)
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::DrawFrame");
    ecss_[ecs] = handles;
    WIDGET_LOGD("ACE-3D DrawFrame ecss size %zu", ecss_.size());
}

void GraphicsManagerCommon::PerformDraw()
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::PerformDraw");
    if (engine_ == nullptr) {
        WIDGET_LOGE("ACE-3D PerformDraw but engine is null");
        return;
    }

    WIDGET_LOGD("ACE-3D PerformDraw");
    engine_->DrawMultiEcs(ecss_);
    engine_->AddTextureMemoryBarrrier();
    ecss_.clear();
}

void GraphicsManagerCommon::AttachContext(const OHOS::Ace::WeakPtr<OHOS::Ace::PipelineBase>& context)
{
    WIDGET_SCOPED_TRACE("GraphicsManagerCommon::AttachContext");
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
