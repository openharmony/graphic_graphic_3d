/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef CORE_GLTF_GLTF2_H
#define CORE_GLTF_GLTF2_H

#include <3d/gltf/gltf.h>
#include <core/namespace.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IEngine;
class IFileManager;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IGraphicsContext;

// implementation of public api..
class Gltf2 final : public IGltf2 {
public:
    explicit Gltf2(IGraphicsContext& graphicsContext);
    // allows for partial initialization. (used by tests)
    explicit Gltf2(CORE_NS::IFileManager& fileManager);
    ~Gltf2() override = default;

    GLTFLoadResult LoadGLTF(BASE_NS::string_view uri) override;
    GLTFLoadResult LoadGLTF(BASE_NS::array_view<uint8_t const> data) override;
    bool SaveGLTF(CORE_NS::IEcs& ecs, BASE_NS::string_view uri) override;
    IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs) override;
    IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs, CORE_NS::IThreadPool& pool) override;
    CORE_NS::Entity ImportGltfScene(size_t sceneIndex, const IGLTFData& gltfData,
        const GLTFResourceData& gltfImportData, CORE_NS::IEcs& ecs, CORE_NS::Entity rootEntity,
        GltfSceneImportFlags flags) override;

private:
    CORE_NS::IEngine* engine_ { nullptr };
    RENDER_NS::IRenderContext* renderContext_ { nullptr };
    CORE_NS::IFileManager& fileManager_;
};
CORE3D_END_NAMESPACE()
#endif // CORE_GLTF_GLTF2_H