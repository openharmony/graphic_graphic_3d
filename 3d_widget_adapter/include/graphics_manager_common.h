/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
#define OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H

#include <unordered_set>

#include <EGL/egl.h>

#include "engine_factory.h"
#if MULTI_ECS_UPDATE_AT_ONCE
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
    EGLContext GetOrCreateOffScreenContext(EGLContext eglContext);
    void BindOffScreenContext();
    virtual PlatformData GetPlatformData() const = 0;
    bool HasMultiEcs();
    RenderBackend GetRenderBackendType(int32_t key);
#if MULTI_ECS_UPDATE_AT_ONCE
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
#if MULTI_ECS_UPDATE_AT_ONCE
    void PerformDraw();
#endif

private:
    std::unordered_set<int32_t> viewTextures_;
    OffScreenContextHelper offScreenContextHelper_;
    std::unique_ptr<IEngine> engine_ = nullptr;
    bool engineLoaded_ = false;
    bool engineInited_ = false;
    std::unordered_map<int32_t, RenderBackend> backends_;
#if MULTI_ECS_UPDATE_AT_ONCE
    std::unordered_map<void*, void*> ecss_;
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
