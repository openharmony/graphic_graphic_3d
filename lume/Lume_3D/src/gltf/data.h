/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

// Implementation of outside-world GLTF data interface.
// Inherits GltfData (public data) and adds internal-only fields.
class Data : public GltfData, public IGLTFData {
public:
    explicit Data(CORE_NS::IFileManager& fileManager);
    bool LoadBuffers() override;
    void ReleaseBuffers() override;

    BASE_NS::vector<BASE_NS::string> GetExternalFileUris() override;

    size_t GetDefaultSceneIndex() const override;
    size_t GetSceneCount() const override;

    size_t GetThumbnailImageCount() const override;
    IGLTFData::ThumbnailImage GetThumbnailImage(size_t thumbnailIndex) override;

    const GltfData& GetData() const override;
    AccessorData ReadAccessorData(const GLTF2::Accessor& accessor) const override;

    CORE_NS::IFile::Ptr memoryFile_;

    // Internal-only fields not in the public GltfData struct.
    int64_t defaultResourcesOffset = -1;
    size_t size{0};
#ifdef GLTF2_EXTENSION_KHR_LIGHTS_PBR
    uint32_t pbrLightOffset{0};
#endif

protected:
    CORE_NS::IFileManager& fileManager_;
    void Destroy() override;
};

class SceneData : public ISceneData {
public:
    static constexpr auto UID = BASE_NS::Uid{"e745a051-2372-4935-8ed1-30dfc0d5f305"};

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

    int32_t refcnt_{0};
    BASE_NS::unique_ptr<GLTF2::Data> data_;
};
}  // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif  // CORE__GLTF__DATA_H
