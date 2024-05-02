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

// clang-format off
#include <meta/interface/object_macros.h>
#include <meta/interface/intf_object_registry.h>
// clang-format on

#include <scene_plugin/api/material.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_environment.h>
#include <scene_plugin/interface/intf_nodes.h>
#include <scene_plugin/interface/intf_scene.h>
#include <scene_plugin/interface/intf_scene_presenter.h>

#include <3d/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/implementation_uids.h>

#include <meta/base/plugin.h>

#include "ecs_animation.h"
#include "intf_node_private.h"

namespace {
static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };
} // namespace

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

using namespace CORE_NS;

SCENE_BEGIN_NAMESPACE()
template<typename T, const META_NS::ClassInfo& Info>
class PendingRequestImpl : public META_NS::ObjectFwd<PendingRequestImpl<T, Info>, Info, META_NS::ClassId::Object,
                               SCENE_NS::IPendingRequest<T>, SCENE_NS::IPendingRequestData<T>> {
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnReady)

    void Add(const T& data) override
    {
        data_.push_back(data);
    }

    void MarkReady() override
    {
        applicationData_.clear();
        META_NS::Invoke<META_NS::IOnChanged>(OnReady());
    }

    const BASE_NS::vector<T>& GetResults() const override
    {
        return data_;
    }

    BASE_NS::vector<T>& MutableData() override
    {
        return data_;
    }

    BASE_NS::vector<BASE_NS::string>& MetaData() override
    {
        return applicationData_;
    }

    BASE_NS::vector<T> data_;
    BASE_NS::vector<BASE_NS::string> applicationData_;
};

void RegisterNodes();
void UnregisterNodes();

void RegisterSceneImpl();
void UnregisterSceneImpl();

void RegisterEcsObject();
void UnregisterEcsObject();

void RegisterNodeHierarchyController();
void UnregisterNodeHierarchyController();

void RegisterEngineAccess();
void UnregisterEngineAccess();

SCENE_END_NAMESPACE()

class SceneBitmap : public META_NS::ObjectFwd<SceneBitmap, SCENE_NS::ClassId::Bitmap, META_NS::ClassId::Object,
                        SCENE_NS::IBitmap, META_NS::INamed> {
protected:
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name)

    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IBitmap, BASE_NS::string, Uri, {})

    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IBitmap, BASE_NS::Math::UVec2, Size,
        BASE_NS::Math::UVec2(0, 0), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_EVENT(SCENE_NS::IBitmap, META_NS::IOnChanged, ResourceChanged)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IBitmap, SCENE_NS::IBitmap::BitmapStatus, Status,
        SCENE_NS::IBitmap::BitmapStatus::NOT_INITIALIZED, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)

    bool Build(const IMetadata::Ptr& data) override
    {
        return true;
    }
    void SetRenderHandle(RENDER_NS::RenderHandleReference handle, const BASE_NS::Math::UVec2 size) override
    {
        auto cs = META_ACCESS_PROPERTY_VALUE(Size);
        if (size != cs) {
            META_ACCESS_PROPERTY(Size)->SetValue(size);
        }
        handle_ = handle;
        if (RENDER_NS::RenderHandleUtil::IsValid(handle.GetHandle())) {
            META_ACCESS_PROPERTY(Status)->SetValue(SCENE_NS::IBitmap::BitmapStatus::COMPLETED);
        } else {
            META_ACCESS_PROPERTY(Status)->SetValue(SCENE_NS::IBitmap::BitmapStatus::NOT_INITIALIZED);
        }
        META_ACCESS_EVENT(ResourceChanged)->Invoke();
    }
    RENDER_NS::RenderHandleReference GetRenderHandle() const override
    {
        return handle_;
    }

    RENDER_NS::RenderHandleReference handle_;
};

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Pluginregistry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;
    auto& objreg = META_NS::GetObjectRegistry();
    objreg.RegisterObjectType<SceneBitmap>();

    SCENE_NS::RegisterEcsObject();
    SCENE_NS::RegisterNodes();
    SCENE_NS::RegisterSceneImpl();
    SCENE_NS::RegisterNodeHierarchyController();
    SCENE_NS::RegisterEngineAccess();

    SCENE_NS::RegisterEcsAnimationObjectType();
    META_NS::GetObjectRegistry()
        .RegisterObjectType<
            SCENE_NS::PendingRequestImpl<SCENE_NS::NodeDistance, SCENE_NS::ClassId::PendingDistanceRequest>>();
    META_NS::GetObjectRegistry()
        .RegisterObjectType<
            SCENE_NS::PendingRequestImpl<RENDER_NS::GraphicsState, SCENE_NS::ClassId::PendingGraphicsStateRequest>>();
    META_NS::GetObjectRegistry()
        .RegisterObjectType<SCENE_NS::PendingRequestImpl<BASE_NS::Math::Vec3, SCENE_NS::ClassId::PendingVec3Request>>();
    return {};
}
void UnregisterInterfaces(PluginToken)
{
    /* META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IBloom>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IBlur>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ICamera>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IColorConversion>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IColorFringe>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IDepthOfField>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IDither>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IEcsAnimation>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IEcsObject>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IEcsScene>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IEcsTrackAnimation>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IEnvironment>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IFxaa>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IGraphicsState>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ILight>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IMaterial>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IMesh>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IMultiMeshProxy>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IModel>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IMotionBlur>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::INode>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IPostProcess>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IPrefab>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IPrefabInstance>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IScene>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IScenePresenter>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IShader>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ISubMesh>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ITaa>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ITextureInfo>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::ITonemap>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IViewNode>();
     META_NS::UnRegisterInterfacePropertyType<SCENE_NS::IVignette>();
 */
    SCENE_NS::UnregisterEcsObject();
    SCENE_NS::UnregisterNodes();
    SCENE_NS::UnregisterSceneImpl();
    SCENE_NS::UnregisterNodeHierarchyController();
    SCENE_NS::UnregisterEngineAccess();

    SCENE_NS::UnregisterEcsAnimationObjectType();

    META_NS::GetObjectRegistry()
        .UnregisterObjectType<
            SCENE_NS::PendingRequestImpl<SCENE_NS::NodeDistance, SCENE_NS::ClassId::PendingDistanceRequest>>();
    META_NS::GetObjectRegistry()
        .UnregisterObjectType<
            SCENE_NS::PendingRequestImpl<RENDER_NS::GraphicsState, SCENE_NS::ClassId::PendingGraphicsStateRequest>>();
    META_NS::GetObjectRegistry()
        .UnregisterObjectType<
            SCENE_NS::PendingRequestImpl<BASE_NS::Math::Vec3, SCENE_NS::ClassId::PendingVec3Request>>();
}
const char* VersionString()
{
    return "1.0";
}

const BASE_NS::Uid plugin_deps[] { RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN,
    META_NS::META_OBJECT_PLUGIN_UID };

extern "C" {
#if _MSC_VER
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    CORE_NS::IPlugin gPluginData { IPlugin::UID, "Scene",
        /** Version information of the plugin. */
        Version { SCENE_NS::UID_SCENE_PLUGIN, VersionString }, RegisterInterfaces, UnregisterInterfaces, plugin_deps };
}
