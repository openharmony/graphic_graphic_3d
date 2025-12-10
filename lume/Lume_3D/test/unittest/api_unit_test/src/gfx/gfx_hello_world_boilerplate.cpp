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

/**
 * Boilerplate code for using the hello world easily in multiple places.
 */
#include "gfx_hello_world_boilerplate.h"

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/render/default_material_constants.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/perf/intf_performance_data_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/util/performance_data_structures.h>

// Helloworld test
#include <3d/util/intf_mesh_util.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace HelloWorldBoilerplate {
void CreateCubeObjects(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();
    const IShaderManager& shaderMgr = res.GetRenderContext().GetDevice().GetShaderManager();
    IEntityManager& em = ecs.GetEntityManager();
    auto materialManager = GetManager<IMaterialComponentManager>(ecs);

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    constexpr float planeSize { 2.0f };

    const Math::Vec3 basePos = { 0.0f, 0.0f, -10.0f };

    auto CreateCube = [&](const Math::Vec4 col, const Math::Vec3 pos, const bool sort, const uint8_t layer,
                          const uint8_t order) {
        Entity materialEntity = em.Create();
        materialManager->Create(materialEntity);
        if (auto handle = materialManager->Write(materialEntity); handle) {
            handle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = col;
            handle->customRenderSlotId =
                shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);

            if (sort) {
                handle->renderSort.renderSortLayer = layer;
                handle->renderSort.renderSortLayerOrder = order;
            }
        }
        const Entity cubeEntity =
            res.GetGraphicsContext().GetMeshUtil().GenerateCube(ecs, "", materialEntity, 2.0f, 2.0f, 2.0f);
        nodeSystem->GetNode(cubeEntity)->SetPosition(pos);
    };
    // sort to correct transparent order (final first)
    CreateCube({ 1.0f, 0.0f, 0.0f, 0.5f }, basePos + Math::Vec3 { -5.0f, 0.0f, 0.0f }, true, 63, 0);
    CreateCube({ 0.0f, 1.0f, 0.0f, 0.5f }, basePos + Math::Vec3 { -5.0f, 0.0f, -1.0f }, true, 1, 0);
    CreateCube({ 0.0f, 0.0f, 1.0f, 0.5f }, basePos + Math::Vec3 { -5.0f, 0.0f, -2.0f }, true, 0, 0);

    // sort to wrong transparent order with center flip
    CreateCube({ 1.0f, 0.0f, 0.0f, 0.5f }, basePos + Math::Vec3 { 5.0f, 0.0f, 0.0f }, true, 32, 0);
    CreateCube({ 0.0f, 1.0f, 0.0f, 0.5f }, basePos + Math::Vec3 { 5.0f, 0.0f, -1.0f }, true, 36, 0);
    CreateCube({ 0.0f, 0.0f, 1.0f, 0.5f }, basePos + Math::Vec3 { 5.0f, 0.0f, -2.0f }, true, 36, 1);
}

/*  Similar test case as we have the Hello world application
    but in this case we do not rotate the cubes but render
    only n number of frames for validation.
    In addition has material render sorting.
*/
void helloWorldTest(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();
    IEntityManager& em = ecs.GetEntityManager();

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    auto sceneManager = GetManager<IRenderConfigurationComponentManager>(ecs);
    sceneManager->Create(sceneRoot);
    auto sceneHandle = sceneManager->Write(sceneRoot);
    RenderConfigurationComponent& renderConfig = *sceneHandle;
    renderConfig.environment = sceneRoot;

    auto evnManager = GetManager<IEnvironmentComponentManager>(ecs);
    evnManager->Create(renderConfig.environment);
    if (auto envDataHandle = evnManager->Write(renderConfig.environment); envDataHandle) {
        EnvironmentComponent& envComponent = *envDataHandle;
        // Update env map type
        envComponent.background = EnvironmentComponent::Background::CUBEMAP;
        envComponent.environmentRotation = Math::AngleAxis((Math::DEG2RAD * -90.0f), Math::Vec3(0.0f, 0.0f, 1.0f));
    }

    const Entity cameraEntity = em.Create();

    GetManager<ILocalMatrixComponentManager>(ecs)->Create(cameraEntity);
    GetManager<INodeComponentManager>(ecs)->Create(cameraEntity);
    GetManager<IWorldMatrixComponentManager>(ecs)->Create(cameraEntity);

    auto tcm = GetManager<ITransformComponentManager>(ecs);
    tcm->Create(cameraEntity);
    tcm->Write(cameraEntity)->position = { 0.0f, 0.0f, 2.5f };

    auto ccm = GetManager<ICameraComponentManager>(ecs);
    ccm->Create(cameraEntity);
    if (auto cameraHandle = ccm->Write(cameraEntity); cameraHandle) {
        CameraComponent& cc = *cameraHandle;
        cc.sceneFlags |=
            CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT | CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        cc.projection = CameraComponent::Projection::PERSPECTIVE;
        cc.yFov = Math::DEG2RAD * 60.0f;
        cc.zNear = 0.1f;
        cc.zFar = 100.0f;
        cc.aspect = static_cast<float>(res.GetWindowWidth()) / static_cast<float>(res.GetWindowHeight());
        cc.renderResolution[0] = static_cast<uint32_t>(res.GetWindowWidth());
        cc.renderResolution[1] = static_cast<uint32_t>(res.GetWindowHeight());
    }

    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);
    auto uriManager = GetManager<IUriComponentManager>(ecs);

    const Entity materialEntity = em.Create();
    materialManager->Create(materialEntity);
    uriManager->Set(materialEntity, { string("test://red_material") });
    nameManager->Set(materialEntity, { string("Red Material") });

    materialManager->Write(materialEntity)->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = { 1.f, 0.f,
        0.f, 1.f };

    const Entity cubeEntity =
        res.GetGraphicsContext().GetMeshUtil().GenerateCube(ecs, "TheCube", materialEntity, 2.0f, 2.0f, 2.0f);
    nodeSystem->GetNode(cubeEntity)->SetPosition(Math::Vec3(0.f, 0.f, -10.f));

    // add material render sort objects
    CreateCubeObjects(res);
}

void GetPerformanceDataCounters(const string_view name, IPerformanceDataManager::CounterPairView data)
{
    if (IPerformanceDataManagerFactory* globalPerfData =
            GetInstance<IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        if (IPerformanceDataManager* perfData = globalPerfData->Get(name); perfData) {
            perfData->GetSelectedCounters(data);
        }
    }
}

IPerformanceDataManager::ComparisonData ComparePerformanceDataCounters(const string_view name,
    const IPerformanceDataManager::ConstCounterPairView lhs, const IPerformanceDataManager::ConstCounterPairView rhs)
{
    IPerformanceDataManager::ComparisonData cd;
    if (IPerformanceDataManagerFactory* globalPerfData =
            GetInstance<IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        if (IPerformanceDataManager* perfData = globalPerfData->Get(name); perfData) {
            cd = perfData->CompareCounters(lhs, rhs);
        }
    }
    return cd;
}
} // namespace HelloWorldBoilerplate
