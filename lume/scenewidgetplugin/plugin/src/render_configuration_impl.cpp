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
#include <scene_plugin/api/render_configuration_uid.h>
#include <scene_plugin/interface/intf_render_configuration.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_resource_private.h"
#include "PropertyHandlerArrayHolder.h"
#include "task_utils.h"

using namespace SCENE_NS;

namespace {

class RenderConfigurationImpl
    : public META_NS::ObjectFwd<RenderConfigurationImpl, SCENE_NS::ClassId::RenderConfiguration,
          META_NS::ClassId::Object, SCENE_NS::IRenderConfiguration, IResourcePrivate, SCENE_NS::IProxyObject> {
    using Fwd = META_NS::ObjectFwd<RenderConfigurationImpl, SCENE_NS::ClassId::RenderConfiguration,
        META_NS::ClassId::Object, SCENE_NS::IRenderConfiguration, IResourcePrivate, SCENE_NS::IProxyObject>;

    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IRenderConfiguration, SCENE_NS::IEnvironment::Ptr, Environment)

    ~RenderConfigurationImpl() {}

    bool Build(const IMetadata::Ptr& data) override
    {
        return Fwd::Build(data);
    }

    void Connect(SceneHolder::Ptr sh) override
    {
        if (ecsObject_) {
            return;
        }

        ecsObject_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IEcsObject>(SCENE_NS::ClassId::EcsObject);
        sh_ = sh;

        if (auto proxyIf = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(ecsObject_)) {
            proxyIf->SetCommonListener(sh->GetCommonEcsListener());
        }

        auto ecs = sh->GetEcs();

        auto renderConfiguration = sh->CreateRenderConfiguration();
        ecsObject_->SetEntity(ecs, renderConfiguration);

        propHandler_.Reset();
        propHandler_.SetSceneHolder(sh);

        if (auto meta = interface_pointer_cast<IMetadata>(ecsObject_)) {
            ConvertBindChanges<IEnvironment::Ptr, CORE_NS::Entity, EntityConverter<SCENE_NS::IEnvironment>>(
                propHandler_, META_ACCESS_PROPERTY(Environment), meta, "RenderConfigurationComponent.environment",
                ONE_WAY_TO_ECS);
        }
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }

    SCENE_NS::IEcsObject::Ptr ecsObject_;
    SceneHolder::Ptr sh_;
    PropertyHandlerArrayHolder propHandler_ {};
};

} // namespace

SCENE_BEGIN_NAMESPACE()

void RegisterRenderConfigurationImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType<RenderConfigurationImpl>();
}

void UnregisterRenderConfigurationImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.UnregisterObjectType<RenderConfigurationImpl>();
}

SCENE_END_NAMESPACE()
