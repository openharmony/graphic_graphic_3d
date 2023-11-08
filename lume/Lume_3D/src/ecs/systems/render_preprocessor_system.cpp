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

#include "render_preprocessor_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_macros.h>

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/scoped_handle.h>
#include <render/implementation_uids.h>

#include "util/component_util_functions.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

BEGIN_PROPERTY(IRenderPreprocessorSystem::Properties, ComponentMetadata)
    DECL_PROPERTY2(
        IRenderPreprocessorSystem::Properties, dataStoreManager, "IRenderDataStoreManager", PropertyFlags::IS_HIDDEN)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreMaterial, "dataStoreMaterial", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreCamera, "dataStoreCamera", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreLight, "dataStoreLight", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreScene, "dataStoreScene", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreMorph, "dataStoreMorph", 0)
END_PROPERTY();

RenderPreprocessorSystem::RenderPreprocessorSystem(IEcs& ecs)
    : ecs_(ecs), RENDER_PREPROCESSOR_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        renderContext_ = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
        if (renderContext_) {
            graphicsContext_ =
                GetInstance<IGraphicsContext>(*renderContext_->GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT);
        }
    }
}

RenderPreprocessorSystem::~RenderPreprocessorSystem()
{
    if (properties_.dataStoreManager) {
        IRenderDataStoreManager& rdsMgr = *properties_.dataStoreManager;
        if (dsScene_) {
            rdsMgr.Destroy(IRenderDataStoreDefaultScene::UID, dsScene_);
        }
        if (dsCamera_) {
            rdsMgr.Destroy(IRenderDataStoreDefaultCamera::UID, dsCamera_);
        }
        if (dsLight_) {
            rdsMgr.Destroy(IRenderDataStoreDefaultLight::UID, dsLight_);
        }
        if (dsMaterial_) {
            rdsMgr.Destroy(IRenderDataStoreDefaultMaterial::UID, dsMaterial_);
        }
        if (dsMorph_) {
            rdsMgr.Destroy(IRenderDataStoreMorph::UID, dsMorph_);
        }
    }
}

void RenderPreprocessorSystem::SetActive(bool state)
{
    active_ = state;
}

bool RenderPreprocessorSystem::IsActive() const
{
    return active_;
}

string_view RenderPreprocessorSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid RenderPreprocessorSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* RenderPreprocessorSystem::GetProperties()
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* RenderPreprocessorSystem::GetProperties() const
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

void RenderPreprocessorSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &RENDER_PREPROCESSOR_SYSTEM_PROPERTIES) {
        return;
    }
    if (const auto in = ScopedHandle<const IRenderPreprocessorSystem::Properties>(&data); in) {
        properties_.dataStoreScene = in->dataStoreScene;
        properties_.dataStoreCamera = in->dataStoreCamera;
        properties_.dataStoreLight = in->dataStoreLight;
        properties_.dataStoreMaterial = in->dataStoreMaterial;
        properties_.dataStoreMorph = in->dataStoreMorph;
        if (properties_.dataStoreManager) {
            SetDataStorePointers(*properties_.dataStoreManager);
        }
    }
}

void RenderPreprocessorSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    properties_.dataStoreManager = &manager;

    // creates own data stores based on names
    dsScene_ = static_cast<IRenderDataStoreDefaultScene*>(
        manager.Create(IRenderDataStoreDefaultScene::UID, properties_.dataStoreScene.data()));
    dsCamera_ = static_cast<IRenderDataStoreDefaultCamera*>(
        manager.Create(IRenderDataStoreDefaultCamera::UID, properties_.dataStoreCamera.data()));
    dsLight_ = static_cast<IRenderDataStoreDefaultLight*>(
        manager.Create(IRenderDataStoreDefaultLight::UID, properties_.dataStoreLight.data()));
    dsMaterial_ = static_cast<IRenderDataStoreDefaultMaterial*>(
        manager.Create(IRenderDataStoreDefaultMaterial::UID, properties_.dataStoreMaterial.data()));
    dsMorph_ = static_cast<IRenderDataStoreMorph*>(
        properties_.dataStoreManager->Create(IRenderDataStoreMorph::UID, properties_.dataStoreMorph.data()));
}

const IEcs& RenderPreprocessorSystem::GetECS() const
{
    return ecs_;
}

void RenderPreprocessorSystem::Initialize()
{
    if (graphicsContext_ && renderContext_) {
        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
    }
}

bool RenderPreprocessorSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    if (!frameRenderingQueued) {
        return false;
    }
    return true;
}

void RenderPreprocessorSystem::Uninitialize() {}

ISystem* IRenderPreprocessorSystemInstance(IEcs& ecs)
{
    return new RenderPreprocessorSystem(ecs);
}

void IRenderPreprocessorSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderPreprocessorSystem*>(instance);
}
CORE3D_END_NAMESPACE()
