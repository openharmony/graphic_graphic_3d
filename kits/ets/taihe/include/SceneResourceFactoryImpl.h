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

#ifndef OHOS_3D_SCENE_RESOURCE_FACTORY_IMPL_H
#define OHOS_3D_SCENE_RESOURCE_FACTORY_IMPL_H

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "SceneTH.Transfer.proj.hpp"
#include "SceneTH.Transfer.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include "SceneResources.user.hpp"

#include "ANIUtils.h"
#include "CameraImpl.h"
#include "EnvironmentImpl.h"
#include "GeometryImpl.h"
#include "LightImpl.h"
#include "MaterialImpl.h"
#include "NodeImpl.h"

#include "RenderResourceFactoryImpl.h"
#include "SceneETS.h"
#include "geometry_definition/GeometryDefinition.h"
#include "MeshResourceETS.h"
#include "MaterialETS.h"
#include "geometry_definition/CubeETS.h"
#include "geometry_definition/CustomETS.h"
#include "geometry_definition/PlaneETS.h"
#include "geometry_definition/SphereETS.h"
#include "geometry_definition/CylinderETS.h"

namespace OHOS::Render3D::KITETS {
class SceneResourceFactoryImpl : public RenderResourceFactoryImpl {
public:
    SceneResourceFactoryImpl(const std::shared_ptr<SceneETS> sceneETS) : sceneETS_(sceneETS)
    {}

    ~SceneResourceFactoryImpl()
    {
        sceneETS_.reset();
    }

    ::SceneNodes::Camera createCameraSync(::SceneTH::SceneNodeParameters const &params);

    ::SceneNodes::Camera createCameraSyncWithCameraParams(
        ::SceneTH::SceneNodeParameters const &params, ::SceneTH::CameraParameters const &cameraParams);

    ::SceneNodes::LightTypeUnion createLightSync(
        ::SceneTH::SceneNodeParameters const &params, ::SceneNodes::LightType lightType);

    ::SceneNodes::Node createNodeSync(::SceneTH::SceneNodeParameters const &params);

    ::SceneResources::VariousMaterial createMaterialSync(
        ::SceneTH::SceneResourceParameters const &params, ::SceneResources::MaterialType materialType);

    ::SceneResources::Environment createEnvironmentSync(::SceneTH::SceneResourceParameters const &params);

    ::SceneNodes::Geometry createGeometrySync(
        ::SceneTH::SceneNodeParameters const &params, ::SceneResources::weak::MeshResource mesh);

    ::SceneResources::Effect createEffectSync(::SceneTH::EffectParameters const &params);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

    std::shared_ptr<SceneETS> getInternalScene() const
    {
        return sceneETS_;
    }

private:
    std::shared_ptr<SceneETS> sceneETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SCENE_RESOURCE_FACTORY_IMPL_H