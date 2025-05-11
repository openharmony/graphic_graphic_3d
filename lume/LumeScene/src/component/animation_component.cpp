
#include "animation_component.h"

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/animation_state_component.h>

SCENE_BEGIN_NAMESPACE()

BASE_NS::string AnimationComponent::GetName() const
{
    return "AnimationComponent";
}

bool AnimationComponent::SetEcsObject(const IEcsObject::Ptr& obj)
{
    bool ret = Super::SetEcsObject(obj);
    if (ret) {
        auto& ecsc = obj->GetScene()->GetEcsContext();
        if (auto m = ecsc.FindComponent<CORE3D_NS::AnimationStateComponent>()) {
            if (!m->HasComponent(GetEcsObject()->GetEntity())) {
                m->Create(GetEcsObject()->GetEntity());
            }
        }
    }
    return ret;
}

SCENE_END_NAMESPACE()
