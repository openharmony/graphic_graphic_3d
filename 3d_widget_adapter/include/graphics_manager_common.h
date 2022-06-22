/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
#define OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H

#include "offscreen_context.h"
#include "i_engine.h"
#include "engine_factory.h"
#if MULTI_ECS_UPDATE_AT_ONCE
#include "frameworks/core/pipeline/pipeline_context.h"
#endif

#include <unordered_map>
#include <GLES3/gl31.h>
#include <EGL/egl.h>

namespace OHOS::Render3D {
struct TextureInfo;
struct PlatformData;
class GraphicsManagerCommon {
public:
    explicit GraphicsManagerCommon(const GraphicsManagerCommon&) = delete;
    GraphicsManagerCommon& operator=(const GraphicsManagerCommon&) = delete;

    void Register(int32_t key);
    void UnRegister(int32_t key);
    std::unique_ptr<IEngine> GetEngine(EngineFactory::EngineType type, EGLContext eglContext);
    TextureInfo CreateRenderTexture(int32_t key, uint32_t width, uint32_t height, EGLContext eglContext);
    virtual PlatformData GetPlatformData() const = 0;
    bool HasMultiEcs();
#if MULTI_ECS_UPDATE_AT_ONCE
    void AttachContext(const OHOS::Ace::WeakPtr<OHOS::Ace::PipelineContext> context);
    void DrawFrame(void* ecs);
#endif

protected:
    GraphicsManagerCommon() = default;
    virtual ~GraphicsManagerCommon();
    bool LoadEngineLib();
    bool InitEngine(EGLContext eglContext, PlatformData data);
    void DeInitEngine();
    void UnLoadEngineLib();
#if MULTI_ECS_UPDATE_AT_ONCE
    void PerformDraw();
#endif

private:
    std::unordered_map<int32_t, GLuint> viewTextureMap_;
    OffScreenContext offScreenContextHelper_;
    std::unique_ptr<IEngine> engine_ = nullptr;
    bool engineLoaded_ = false;
    bool engineInited_ = false;
#if MULTI_ECS_UPDATE_AT_ONCE
    std::vector<void* > ecss_;
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHICS_MANAGER_COMMON_H
