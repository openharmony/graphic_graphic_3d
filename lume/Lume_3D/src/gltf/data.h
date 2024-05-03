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

#ifndef CORE__GLTF__DATA_H
#define CORE__GLTF__DATA_H

#include <3d/gltf/gltf.h>
#include <3d/loaders/intf_scene_loader.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/io/intf_file.h>

#include "gltf/gltf2_data_structures.h"

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
struct Assets {
    Assets() = default;
    Assets(const Assets& aOther) = delete;
    virtual ~Assets() = default;

    BASE_NS::string filepath;
    BASE_NS::string defaultResources;
    int32_t defaultResourcesOffset = -1;

    size_t size { 0 };

    BASE_NS::unique_ptr<GLTF2::Material> defaultMaterial;
    BASE_NS::unique_ptr<GLTF2::Sampler> defaultSampler;
    GLTF2::Scene* defaultScene { nullptr };

    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Buffer>> buffers;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::BufferView>> bufferViews;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Accessor>> accessors;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Mesh>> meshes;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Camera>> cameras;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Image>> images;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Sampler>> samplers;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Texture>> textures;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Material>> materials;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Node>> nodes;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Scene>> scenes;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Animation>> animations;
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::Skin>> skins;

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)

#ifdef GLTF2_EXTENSION_KHR_LIGHTS_PBR
    uint32_t pbrLightOffset; // whats this? (seems to be a parse time helper, index to first pbr light)
#endif

    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::KHRLight>> lights;
#endif

#if defined(GLTF2_EXTENSION_HW_XR_EXT)
    struct Thumbnail {
        BASE_NS::string uri;
        BASE_NS::string extension;
        BASE_NS::vector<uint8_t> data;
    };
    BASE_NS::vector<Thumbnail> thumbnails;
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    BASE_NS::vector<BASE_NS::unique_ptr<GLTF2::ImageBasedLight>> imageBasedLights;
#endif

#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
    // true then KHR_mesh_quantization extension required. this expands the valid attribute componentTypes.
    bool quantization { false };
#endif
};

// Implementation of outside-world GLTF data interface.
class Data : public Assets, public IGLTFData {
public:
    explicit Data(CORE_NS::IFileManager& fileManager);
    bool LoadBuffers() override;
    void ReleaseBuffers() override;

    BASE_NS::vector<BASE_NS::string> GetExternalFileUris() override;

    size_t GetDefaultSceneIndex() const override;
    size_t GetSceneCount() const override;

    size_t GetThumbnailImageCount() const override;
    IGLTFData::ThumbnailImage GetThumbnailImage(size_t thumbnailIndex) override;

    CORE_NS::IFile::Ptr memoryFile_;

protected:
    CORE_NS::IFileManager& fileManager_;
    void Destroy() override;
};

class SceneData : public ISceneData {
public:
    static constexpr auto UID = BASE_NS::Uid { "e745a051-2372-4935-8ed1-30dfc0d5f305" };

    using Ptr = BASE_NS::refcnt_ptr<SceneData>;

    SceneData(BASE_NS::unique_ptr<GLTF2::Data> data);

    const GLTF2::Data* GetData() const;

    size_t GetDefaultSceneIndex() const override;
    size_t GetSceneCount() const override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

protected:
    friend Ptr;

    uint32_t refcnt_ { 0 };
    BASE_NS::unique_ptr<GLTF2::Data> data_;
};
} // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__DATA_H
