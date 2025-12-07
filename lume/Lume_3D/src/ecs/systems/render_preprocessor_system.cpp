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

#include <algorithm>

#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/scoped_handle.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

PROPERTY_LIST(IRenderPreprocessorSystem::Properties, ComponentMetadata,
    MEMBER_PROPERTY(dataStoreScene, "dataStoreScene", 0), MEMBER_PROPERTY(dataStoreCamera, "dataStoreCamera", 0),
    MEMBER_PROPERTY(dataStoreLight, "dataStoreLight", 0), MEMBER_PROPERTY(dataStoreMaterial, "dataStoreMaterial", 0),
    MEMBER_PROPERTY(dataStoreMorph, "dataStoreMorph", 0), MEMBER_PROPERTY(dataStorePrefix, "", 0));

namespace {
static constexpr string_view DEFAULT_DS_SCENE_NAME { "RenderDataStoreDefaultScene" };
static constexpr string_view DEFAULT_DS_CAMERA_NAME { "RenderDataStoreDefaultCamera" };
static constexpr string_view DEFAULT_DS_LIGHT_NAME { "RenderDataStoreDefaultLight" };
static constexpr string_view DEFAULT_DS_MATERIAL_NAME { "RenderDataStoreDefaultMaterial" };
static constexpr string_view DEFAULT_DS_MORPH_NAME { "RenderDataStoreMorph" };

template<typename DataStoreType>
inline auto CreateIfNeeded(
    IRenderDataStoreManager& manager, refcnt_ptr<DataStoreType>& dataStore, string_view dataStoreName)
{
    if (!dataStore || dataStore->GetName() != dataStoreName) {
        dataStore = refcnt_ptr<DataStoreType>(manager.Create(DataStoreType::UID, dataStoreName.data()));
    }
    return dataStore;
}
} // namespace

RenderPreprocessorSystem::RenderPreprocessorSystem(IEcs& ecs)
    : ecs_(ecs), RENDER_PREPROCESSOR_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>()) {
        if (auto* engineClassRegister = engine->GetInterface<IClassRegister>()) {
            renderContext_ = GetInstance<IRenderContext>(*engineClassRegister, UID_RENDER_CONTEXT);
        }
    }
    if (renderContext_) {
        if (auto* renderClassRegister = renderContext_->GetInterface<IClassRegister>()) {
            graphicsContext_ = GetInstance<IGraphicsContext>(*renderClassRegister, UID_GRAPHICS_CONTEXT);
        }
    }
}

RenderPreprocessorSystem::~RenderPreprocessorSystem() = default;

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
        properties_.dataStorePrefix = in->dataStorePrefix;
        properties_.dataStoreScene = in->dataStoreScene;
        properties_.dataStoreCamera = in->dataStoreCamera;
        properties_.dataStoreLight = in->dataStoreLight;
        properties_.dataStoreMaterial = in->dataStoreMaterial;
        properties_.dataStoreMorph = in->dataStoreMorph;

        const bool hasDefaultNames = CheckIfDefaultDataStoreNames();
        if ((properties_.dataStorePrefix.empty()) && (ecs_.GetId() != 0) && hasDefaultNames) {
            properties_.dataStorePrefix = to_string(ecs_.GetId());
        }
        if (hasDefaultNames) {
            properties_.dataStoreScene = properties_.dataStorePrefix + properties_.dataStoreScene;
            properties_.dataStoreCamera = properties_.dataStorePrefix + properties_.dataStoreCamera;
            properties_.dataStoreLight = properties_.dataStorePrefix + properties_.dataStoreLight;
            properties_.dataStoreMaterial = properties_.dataStorePrefix + properties_.dataStoreMaterial;
            properties_.dataStoreMorph = properties_.dataStorePrefix + properties_.dataStoreMorph;
        }

        if (renderContext_) {
            SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
        }
    }
}

void RenderPreprocessorSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    // creates own data stores based on names (the data store name will have the prefix)
    dsScene_ = CreateIfNeeded(manager, dsScene_, properties_.dataStoreScene);
    dsCamera_ = CreateIfNeeded(manager, dsCamera_, properties_.dataStoreCamera);
    dsLight_ = CreateIfNeeded(manager, dsLight_, properties_.dataStoreLight);
    dsMaterial_ = CreateIfNeeded(manager, dsMaterial_, properties_.dataStoreMaterial);
    dsMorph_ = CreateIfNeeded(manager, dsMorph_, properties_.dataStoreMorph);
}

const IEcs& RenderPreprocessorSystem::GetECS() const
{
    return ecs_;
}

bool RenderPreprocessorSystem::CheckIfDefaultDataStoreNames() const
{
    if ((properties_.dataStoreScene == DEFAULT_DS_SCENE_NAME) &&
        (properties_.dataStoreCamera == DEFAULT_DS_CAMERA_NAME) &&
        (properties_.dataStoreLight == DEFAULT_DS_LIGHT_NAME) &&
        (properties_.dataStoreMaterial == DEFAULT_DS_MATERIAL_NAME) &&
        (properties_.dataStoreMorph == DEFAULT_DS_MORPH_NAME)) {
        return true;
    } else {
        return false;
    }
}

void RenderPreprocessorSystem::Initialize()
{
    if (graphicsContext_ && renderContext_) {
        const bool hasDefaultNames = CheckIfDefaultDataStoreNames();
        if ((properties_.dataStorePrefix.empty()) && (ecs_.GetId() != 0) && hasDefaultNames) {
            properties_.dataStorePrefix = to_string(ecs_.GetId());
        }
        if (hasDefaultNames) {
            properties_.dataStoreScene = properties_.dataStorePrefix + properties_.dataStoreScene;
            properties_.dataStoreCamera = properties_.dataStorePrefix + properties_.dataStoreCamera;
            properties_.dataStoreLight = properties_.dataStorePrefix + properties_.dataStoreLight;
            properties_.dataStoreMaterial = properties_.dataStorePrefix + properties_.dataStoreMaterial;
            properties_.dataStoreMorph = properties_.dataStorePrefix + properties_.dataStoreMorph;
        }
        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
    }
}

bool RenderPreprocessorSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    return false;
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
