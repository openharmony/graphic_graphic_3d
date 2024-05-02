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
#include "resource_container.h"

#include <scene_plugin/api/resource_container_uid.h>
#include <scene_plugin/interface/intf_resource_container.h>

#include <meta/ext/attachment/attachment.h>
#include <meta/ext/concrete_base_object.h>

using namespace SCENE_NS;

namespace {
class ResourceContainer
    : public META_NS::AttachmentFwd<ResourceContainer, ClassId::ResourceContainer, IResourceContainer> {
    using Super = META_NS::AttachmentFwd<ResourceContainer, ClassId::ResourceContainer, IResourceContainer>;
    using Super::Super;

    bool Build(const IMetadata::Ptr& data) override
    {        
        if (auto p = META_NS::ConstructProperty<META_NS::IContainer::Ptr>("Resources")) {
            auto container = META_NS::GetObjectRegistry().Create<META_NS::IContainer>(META_NS::ClassId::ObjectFlatContainer);
            p->SetValue(container);
            AddProperty(p);            
        }

        return true;
    }

    META_NS::IContainer::Ptr GetResources() override
    {
        auto property = GetPropertyByName("Resources");
        if (META_NS::Property<META_NS::IContainer::Ptr> cp { property }) {
            return GetValue(cp);
        }
        return {};
    }

    bool AttachTo(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        return true;
    }

    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }

private:
    META_NS::IContainer::Ptr container_;
};

} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterResourceContainerImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<ResourceContainer>();
}
void UnregisterResourceContainerImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<ResourceContainer>();
}
SCENE_END_NAMESPACE()
