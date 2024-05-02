/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef CORE_GLTF_GLTF2_H
#define CORE_GLTF_GLTF2_H

#include <3d/gltf/gltf.h>
#include <3d/loaders/intf_scene_loader.h>
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
class Gltf2 final : public IGltf2, ISceneLoader {
public:
    explicit Gltf2(IGraphicsContext& graphicsContext);
    // allows for partial initialization. (used by tests)
    explicit Gltf2(CORE_NS::IFileManager& fileManager);
    ~Gltf2() override = default;

    // IGltf2
    GLTFLoadResult LoadGLTF(BASE_NS::string_view uri) override;
    GLTFLoadResult LoadGLTF(BASE_NS::array_view<uint8_t const> data) override;
    bool SaveGLTF(CORE_NS::IEcs& ecs, BASE_NS::string_view uri) override;
    IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs) override;
    IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs, CORE_NS::IThreadPool& pool) override;
    CORE_NS::Entity ImportGltfScene(size_t sceneIndex, const IGLTFData& gltfData,
        const GLTFResourceData& gltfImportData, CORE_NS::IEcs& ecs, CORE_NS::Entity rootEntity,
        GltfSceneImportFlags flags) override;

    // ISceneLoader
    Result Load(BASE_NS::string_view uri) override;
    ISceneImporter::Ptr CreateSceneImporter(CORE_NS::IEcs& ecs) override;
    ISceneImporter::Ptr CreateSceneImporter(CORE_NS::IEcs& ecs, CORE_NS::IThreadPool& pool) override;
    BASE_NS::array_view<const BASE_NS::string_view> GetSupportedExtensions() const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    CORE_NS::IEngine* engine_ { nullptr };
    RENDER_NS::IRenderContext* renderContext_ { nullptr };
    CORE_NS::IFileManager& fileManager_;
};
inline constexpr BASE_NS::string_view GetName(const ISceneLoader*)
{
    return "ISceneLoader";
}
CORE3D_END_NAMESPACE()
#endif // CORE_GLTF_GLTF2_H