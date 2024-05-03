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

#ifndef OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
#define OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H

#include <unordered_set>

#include <EGL/egl.h>

#include "engine_factory.h"
#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
#include "frameworks/core/pipeline/pipeline_context.h"
#endif
#include "i_engine.h"
#include "offscreen_context_helper.h"

namespace OHOS::Render3D {
struct TextureInfo;
struct PlatformData;

enum class RenderBackend {
    UNDEFINE,
    GLES,
    GL,
    VULKAN,
    METAL,
};

class __attribute__((visibility("default"))) GraphicsManagerCommon {
public:
    explicit GraphicsManagerCommon(const GraphicsManagerCommon&) = delete;
    GraphicsManagerCommon& operator=(const GraphicsManagerCommon&) = delete;

    void Register(int32_t key, RenderBackend backend = RenderBackend::GLES);
    void UnRegister(int32_t key);
    std::unique_ptr<IEngine> GetEngine(EngineFactory::EngineType type, int32_t key);
    std::unique_ptr<IEngine> GetEngine(EngineFactory::EngineType type, int32_t key, const HapInfo& hapInfo);
    EGLContext GetOrCreateOffScreenContext(EGLContext eglContext);
    void BindOffScreenContext();
    virtual PlatformData GetPlatformData() const = 0;
    virtual PlatformData GetPlatformData(const HapInfo& hapInfo) const = 0;
    const HapInfo& GetHapInfo() const;
    bool HasMultiEcs();
    RenderBackend GetRenderBackendType(int32_t key);
#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void AttachContext(const OHOS::Ace::WeakPtr<OHOS::Ace::PipelineContext> context);
    void DrawFrame(void* ecs, void* handles);
    void UnloadEcs(void* ecs);
#endif

protected:
    GraphicsManagerCommon() = default;
    virtual ~GraphicsManagerCommon();
    bool LoadEngineLib();
    bool InitEngine(EGLContext eglContext, PlatformData data);
    void DeInitEngine();
    void UnloadEngineLib();
#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void PerformDraw();
#endif

private:
    std::unordered_set<int32_t> viewTextures_;
    OffScreenContextHelper offScreenContextHelper_;
    std::unique_ptr<IEngine> engine_ = nullptr;
    bool engineLoaded_ = false;
    bool engineInited_ = false;
    std::unordered_map<int32_t, RenderBackend> backends_;
    HapInfo hapInfo_;
#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    std::unordered_map<void*, void*> ecss_;
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
