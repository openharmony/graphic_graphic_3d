#ifndef SCENE_SRC_CORE_ECS_OBJECT_H
#define SCENE_SRC_CORE_ECS_OBJECT_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_event_listener.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_engine_property_init.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_property.h>
#include <shared_mutex>

#include <core/ecs/intf_ecs.h>

#include <meta/ext/object.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_named.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(EcsObject, "6f1d5812-ce80-4f1d-9a74-6815f3e111b0", META_NS::ObjectCategoryBits::NO_CATEGORY)

class EcsObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IEcsObject, IEcsEventListener,
                      IEnginePropertyInit, META_NS::INamed> {
    META_OBJECT(EcsObject, SCENE_NS::ClassId::EcsObject, IntroduceInterfaces)

    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name, "NameComponent.name")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

public:
    BASE_NS::string GetName() const override;
    IInternalScene::Ptr GetScene() const override;
    CORE_NS::Entity GetEntity() const override;
    META_NS::IEngineValueManager::Ptr GetEngineValueManager() override;

    Future<bool> AddAllProperties(const META_NS::IMetadata::Ptr& object, BASE_NS::string_view component) override;
    Future<bool> AttachProperty(const META_NS::IProperty::Ptr&, BASE_NS::string_view path) override;
    Future<META_NS::IProperty::Ptr> CreateProperty(BASE_NS::string_view path) override;

    Future<META_NS::IProperty::Ptr> CreateProperty(const META_NS::IEngineValue::Ptr&) override;
    virtual Future<bool> AttachProperty(const META_NS::IProperty::Ptr&, const META_NS::IEngineValue::Ptr&) override;

public: // call in engine thread
    bool Build(const META_NS::IMetadata::Ptr& d) override;
    void SyncProperties() override;
    BASE_NS::string GetPath() const override;
    bool SetName(const BASE_NS::string& name) override;

protected:
    void AddPropertyUpdateHook(const META_NS::IProperty::Ptr& p);

    void OnEntityEvent(CORE_NS::IEcs::EntityListener::EventType type) override;
    void OnComponentEvent(
        CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& component) override;

    META_NS::EnginePropertyHandle GetPropertyHandle(BASE_NS::string_view component) const;
    bool AddEngineProperty(META_NS::IMetadata& object, const META_NS::IEngineValue::Ptr& v);
    void AddAllComponentProperties(META_NS::IMetadata& object);
    void AddAllProperties(META_NS::IMetadata& object, CORE_NS::IComponentManager*);
    bool AddAllNamedComponentProperties(META_NS::IMetadata& object, BASE_NS::string_view cv);

    bool AttachEnginePropertyImpl(
        META_NS::EnginePropertyHandle, const BASE_NS::string&, const META_NS::IProperty::Ptr&, BASE_NS::string_view);

    bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    Future<bool> SetActive(bool active) override;

private:
    IInternalScene::WeakPtr scene_;
    CORE_NS::Entity entity_;
    META_NS::IEngineValueManager::Ptr valueManager_;
};

SCENE_END_NAMESPACE()

#endif
