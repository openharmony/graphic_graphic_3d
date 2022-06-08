/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef GRAPHICS_CONTEXT_H
#define GRAPHICS_CONTEXT_H

#include <3d/intf_graphics_context.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IRenderDataStore;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IMeshUtil;
class MeshUtil;
class IGltf2;
class Gltf2;
class ISceneUtil;
class SceneUtil;
class IRenderUtil;
class RenderUtil;

class GraphicsContext final : public IGraphicsContext {
public:
    explicit GraphicsContext(struct Agp3DPluginState&, RENDER_NS::IRenderContext& engine);
    ~GraphicsContext() override;

    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    void Init() override;

    RENDER_NS::IRenderContext& GetRenderContext() const override;

    BASE_NS::array_view<const RENDER_NS::RenderHandleReference> GetRenderNodeGraphs(
        const CORE_NS::IEcs& ecs) const override;

    ISceneUtil& GetSceneUtil() const override;

    IMeshUtil& GetMeshUtil() const override;

    IGltf2& GetGltf() const override;

    IRenderUtil& GetRenderUtil() const override;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    struct Agp3DPluginState& factory_;
    RENDER_NS::IRenderContext& context_;

    BASE_NS::unique_ptr<SceneUtil> sceneUtil_;
    BASE_NS::unique_ptr<MeshUtil> meshUtil_;
    BASE_NS::unique_ptr<Gltf2> gltf2_;
    BASE_NS::unique_ptr<RenderUtil> renderUtil_;
    BASE_NS::vector<RENDER_NS::RenderHandleReference> defaultGpuResources_;
    bool initialized_ { false };
    uint32_t refcnt_ { 0 };
};
CORE3D_END_NAMESPACE()

#endif // GRAPHICS_CONTEXT_H
