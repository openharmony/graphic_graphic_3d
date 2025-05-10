
#ifndef SCENE_SRC_RESOURCE_ENVIRONMENT_H
#define SCENE_SRC_RESOURCE_ENVIRONMENT_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/resource/types.h>
#include <shared_mutex>

#include <meta/api/make_callback.h>
#include <meta/api/resource/derived_from_resource.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/resource/intf_resource.h>

#include "../component/environment_component.h"

SCENE_BEGIN_NAMESPACE()

class Environment : public META_NS::IntroduceInterfaces<META_NS::DerivedFromResource, EnvironmentComponent,
                        META_NS::INamed, ICreateEntity, META_NS::Resource> {
    META_OBJECT(Environment, SCENE_NS::ClassId::Environment, IntroduceInterfaces)
public:
    Environment() : Super(ClassId::EnvironmentResourceTemplate) {}

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

    bool SetEcsObject(const IEcsObject::Ptr& obj) override;
    BASE_NS::string GetName() const override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::EnvironmentResource.Id().ToUid();
    }
};

SCENE_END_NAMESPACE()

#endif