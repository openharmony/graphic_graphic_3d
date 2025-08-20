/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "MeshResourceETS.h"
#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D {
MeshResourceETS::MeshResourceETS(const std::string &name, const std::string &uri,
    BASE_NS::unique_ptr<Geometry::GeometryDefinition> geometryDefinition)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::MESH_RESOURCE)
{
    WIDGET_LOGI("MeshResourceETS ++, name: %{public}s, uri:%{public}s", name.c_str(), uri.c_str());
    geometryDefinition_ = std::move(geometryDefinition);
    SetName(name);
    SetUri(uri);
}

MeshResourceETS::~MeshResourceETS()
{
    WIDGET_LOGI("MeshResourceETS --");
    geometryDefinition_.reset();
}

META_NS::IObject::Ptr MeshResourceETS::GetNativeObj() const
{
    // MeshResource holds no engine resource
    return {};
}

SCENE_NS::IMesh::Ptr MeshResourceETS::CreateMesh(const SCENE_NS::IScene::Ptr &scene) const
{
    if (!scene || !geometryDefinition_) {
        return {};
    }

    const auto meshCreator = scene->CreateObject<SCENE_NS::ICreateMesh>(SCENE_NS::ClassId::MeshCreator).GetResult();
    // Name and material aren't set here. Name is set in the constructor. Material needs to be manually set later.
    auto meshConfig = SCENE_NS::MeshConfig{};
    return geometryDefinition_->CreateMesh(meshCreator, meshConfig);
}
}  // namespace OHOS::Render3D