
#ifndef SCENE_SRC_RENDER_CONFIGURATION_H
#define SCENE_SRC_RENDER_CONFIGURATION_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/intf_render_configuration.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_named.h>

SCENE_BEGIN_NAMESPACE()

class RenderConfiguration : public META_NS::IntroduceInterfaces<EcsLazyProperty, IRenderConfiguration, ICreateEntity> {
    META_OBJECT(RenderConfiguration, ClassId::RenderConfiguration, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(
        IRenderConfiguration, IEnvironment::Ptr, Environment, "RenderConfigurationComponent.environment")
    SCENE_STATIC_PROPERTY_DATA(IRenderConfiguration, BASE_NS::string, RenderNodeGraphUri,
        "RenderConfigurationComponent.customRenderNodeGraphFile")
    SCENE_STATIC_PROPERTY_DATA(IRenderConfiguration, BASE_NS::string, PostRenderNodeGraphUri,
        "RenderConfigurationComponent.customPostSceneRenderNodeGraphFile")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(IEnvironment::Ptr, Environment)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, RenderNodeGraphUri)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, PostRenderNodeGraphUri)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
};
SCENE_END_NAMESPACE()

#endif