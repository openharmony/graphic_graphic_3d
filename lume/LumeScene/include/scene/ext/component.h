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

#ifndef SCENE_EXT_COMPONENT_H
#define SCENE_EXT_COMPONENT_H

#include <scene/base/namespace.h>
#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/scene_property.h>
#include <scene/ext/util.h>

#include <meta/api/engine/util.h>

SCENE_BEGIN_NAMESPACE()

class Component : public META_NS::IntroduceInterfaces<EcsLazyProperty, IComponent, META_NS::IAttachable> {
    using Super = IntroduceInterfaces;

public:
    using IComponent::GetName;

    bool PopulateAllProperties() override
    {
        if (populated_) {
            return true;
        }
        populated_ =
            this->object_->AddAllProperties(this->template GetSelf<META_NS::IMetadata>(), this->GetName()).GetResult();
        return populated_;
    }

    using IMetadata::GetProperty;
    META_NS::IProperty::Ptr GetProperty(BASE_NS::string_view name, META_NS::MetadataQuery q) override
    {
        auto p = Super::GetProperty(name, q);
        if (!p && this->object_) {
            p = this->object_->ResolveProperty(GetSelf<META_NS::IMetadata>(), name, q).GetResult();
        }
        return p;
    }
    META_NS::IProperty::ConstPtr GetProperty(BASE_NS::string_view name, META_NS::MetadataQuery q) const override
    {
        auto p = Super::GetProperty(name, q);
        if (!p && this->object_) {
            p = this->object_->ResolveProperty(GetSelf<META_NS::IMetadata>(), name, q).GetResult();
        }
        return p;
    }

    BASE_NS::vector<META_NS::EnginePropertyInfo> EnumerateProperties(
        const META_NS::EnginePropertyInfoConfig& config) override
    {
        BASE_NS::vector<META_NS::EnginePropertyInfo> result;
        if (auto scene = this->GetInternalScene()) {
            scene->RunDirectlyOrInTask([this, &result, scene, config] {
                auto compName = SCENE_NS::ComponentName(this->GetName());
                if (auto m = scene->GetEcsContext().FindComponent(compName, this->GetEntity())) {
                    META_NS::EnumerateEngineProperties(*m, compName, result, config);
                }
                return true;
            });
        }
        return result;
    }

    IObject::Ptr Resolve(const META_NS::RefUri& uri) const override
    {
        if (uri == META_NS::RefUri::ParentUri()) {
            return interface_pointer_cast<IObject>(parent_);
        }
        return Super::Resolve(uri);
    }

    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        parent_ = target;
        return true;
    }
    bool Detaching(const IAttach::Ptr& target) override
    {
        parent_.reset();
        return true;
    }

private:
    bool populated_{};
    IAttach::WeakPtr parent_;
};

inline META_NS::IProperty::ConstPtr GetComponentProperty(
    const META_NS::IAttach* obj, BASE_NS::string_view component, BASE_NS::string_view name)
{
    if (auto c = obj->GetAttachmentContainer(true)->FindByName(component)) {
        if (auto m = interface_cast<META_NS::IMetadata>(c)) {
            return m->GetProperty(name);
        }
    }
    return nullptr;
}

inline META_NS::IProperty::Ptr GetComponentProperty(
    META_NS::IAttach* obj, BASE_NS::string_view component, BASE_NS::string_view name)
{
    if (auto c = obj->GetAttachmentContainer(true)->FindByName(component)) {
        if (auto m = interface_cast<META_NS::IMetadata>(c)) {
            return m->GetProperty(name);
        }
    }
    return nullptr;
}

#define SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component)        \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                       \
        return GetComponentProperty(this, component, #name);                \
    }

#define SCENE_USE_COMPONENT_PROPERTY(type, name, component)      \
    SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component) \
    ::META_NS::IProperty::Ptr Property##name() noexcept override \
    {                                                            \
        return GetComponentProperty(this, component, #name);     \
    }

// the type is not used, so just forward for now
#define SCENE_USE_ARRAY_COMPONENT_PROPERTY(type, name, component) SCENE_USE_COMPONENT_PROPERTY(type, name, component)

#define SCENE_USE_READONLY_ARRAY_COMPONENT_PROPERTY(type, name, component) \
    SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component)

SCENE_END_NAMESPACE()

#endif
