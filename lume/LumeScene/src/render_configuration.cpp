
#include "render_configuration.h"

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/render_configuration_component.h>
#include <core/ecs/entity.h>

#include "entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity RenderConfiguration::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto& ecs = scene->GetEcsContext();
    auto rconfig = ecs.FindComponent<CORE3D_NS::RenderConfigurationComponent>();
    if (!rconfig) {
        return CORE_NS::Entity {};
    }
    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    rconfig->Create(ent);
    return ent;
}

bool RenderConfiguration::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "Environment") {
        auto ep = object_->CreateProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new InterfacePtrEntityValue<IEnvironment>(ep, { object_->GetScene(), ClassId::Environment })));
    }
    return AttachEngineProperty(p, path);
}

SCENE_END_NAMESPACE()
