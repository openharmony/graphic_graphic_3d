
#ifndef SCENE_EXT_ECS_LAZY_PROPERTY_H
#define SCENE_EXT_ECS_LAZY_PROPERTY_H

#include <scene/base/namespace.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_engine_property_init.h>
#include <scene/ext/util.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class EcsLazyProperty
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IEcsObjectAccess, IEnginePropertyInit> {
public:
    bool SetEcsObject(const IEcsObject::Ptr& obj) override
    {
        object_ = obj;
        return object_ != nullptr;
    }
    IEcsObject::Ptr GetEcsObject() const override
    {
        return object_;
    }
    IInternalScene::Ptr GetInternalScene() const
    {
        return object_ ? object_->GetScene() : nullptr;
    }
    CORE_NS::Entity GetEntity() const
    {
        return object_ ? object_->GetEntity() : CORE_NS::Entity {};
    }

protected:
    bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        return object_->AttachProperty(p, path).GetResult();
    }
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        return AttachEngineProperty(p, path);
    }

protected:
    IEcsObject::Ptr object_;
};

class NamedSceneObject : public META_NS::IntroduceInterfaces<EcsLazyProperty, META_NS::INamed> {
    META_OBJECT_NO_CLASSINFO(NamedSceneObject, IntroduceInterfaces)

    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_FORWARD_PROPERTY(BASE_NS::string, Name, EcsNameProperty())

public:
    META_NS::IProperty::Ptr EcsNameProperty() const
    {
        auto i = interface_cast<META_NS::INamed>(object_);
        return i ? i->Name() : nullptr;
    }

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }
};

SCENE_END_NAMESPACE()

#endif
