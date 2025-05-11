#include "environment.h"

#include <3d/ecs/components/environment_component.h>
#include <core/ecs/intf_entity_manager.h>

#include <meta/interface/resource/intf_object_resource.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity Environment::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto& ecs = scene->GetEcsContext();
    auto envm = ecs.FindComponent<CORE3D_NS::EnvironmentComponent>();
    if (!envm) {
        return CORE_NS::Entity {};
    }
    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    envm->Create(ent);
    return ent;
}

bool Environment::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto ret = Super::SetEcsObject(obj);
    if (ret) {
        Name()->SetValue(GetName());
        Name()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] {
            if (this->object_) {
                this->object_->SetName(Name()->GetValue());
            }
        }));
    }
    return ret;
}

BASE_NS::string Environment::GetName() const
{
    return this->object_ ? this->object_->GetName() : "";
}

SCENE_END_NAMESPACE()
