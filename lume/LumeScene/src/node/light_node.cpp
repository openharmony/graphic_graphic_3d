
#include "light_node.h"

#include <3d/ecs/components/light_component.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity LightNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& sceneUtil = scene->GetGraphicsContext().GetSceneUtil();
    CORE3D_NS::LightComponent lc;
    return sceneUtil.CreateLight(*scene->GetEcsContext().GetNativeEcs(), lc, BASE_NS::Math::Vec3(0.0f, 0.0f, 0.0f),
        BASE_NS::Math::Quat(0.f, 0.f, 0.f, 1.f));
}

bool LightNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<ILight>();
        if (!att.empty()) {
            return true;
        }
    }
    return false;
}

SCENE_END_NAMESPACE()