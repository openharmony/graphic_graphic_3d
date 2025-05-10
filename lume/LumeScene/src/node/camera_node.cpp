
#include "camera_node.h"

#include <3d/ecs/components/camera_component.h>

SCENE_BEGIN_NAMESPACE()

bool CameraNode::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

CORE_NS::Entity CameraNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext();
    auto ccm = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(*ecs.GetNativeEcs());
    if (!ccm) {
        return {};
    }

    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    ecs.AddDefaultComponents(ent);

    CORE3D_NS::CameraComponent cc;
    cc.sceneFlags |= CORE3D_NS::CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
    ccm->Set(ent, cc);

    return ent;
}

bool CameraNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<ICamera>();
        if (!att.empty()) {
            camera_ = att.front();
            return true;
        }
    }
    return false;
}

Future<bool> CameraNode::SetActive(bool active)
{
    CORE_ASSERT(camera_);
    return camera_->SetActive(active);
}
bool CameraNode::IsActive() const
{
    CORE_ASSERT(camera_);
    return camera_->IsActive();
}
Future<bool> CameraNode::SetRenderTarget(const IRenderTarget::Ptr& target)
{
    CORE_ASSERT(camera_);
    return camera_->SetRenderTarget(target);
}

Future<NodeHits> CameraNode::CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->CastRay(pos, options);
    }
    return {};
}
void CameraNode::SendInputEvent(PointerEvent& event)
{
    CORE_ASSERT(camera_);
    camera_->SendInputEvent(event);
}
Future<BASE_NS::Math::Vec3> CameraNode::ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->ScreenPositionToWorld(pos);
    }
    return {};
}
Future<BASE_NS::Math::Vec3> CameraNode::WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->WorldPositionToScreen(pos);
    }
    return {};
}

SCENE_END_NAMESPACE()
