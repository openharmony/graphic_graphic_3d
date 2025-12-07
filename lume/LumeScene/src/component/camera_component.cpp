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

#include "camera_component.h"

#include <scene/ext/util.h>

#include <3d/ecs/components/camera_component.h>

#include <meta/interface/intf_event.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

#include "../core/camera.h"
#include "../core/intf_internal_raycast.h"
#include "../entity_converting_value.h"
#include "../postprocess/postprocess.h"

META_TYPE(CORE3D_NS::CameraComponent::TargetUsage)

SCENE_BEGIN_NAMESPACE()

namespace {
struct FormatConverter {
    using SourceType = ColorFormat;
    using TargetType = CORE3D_NS::CameraComponent::TargetUsage;

    static SourceType ConvertToSource(META_NS::IAny&, const TargetType& v)
    {
        return ColorFormat { v.format, v.usageFlags };
    }
    static TargetType ConvertToTarget(const SourceType& v)
    {
        return CORE3D_NS::CameraComponent::TargetUsage { v.format, v.usageFlags };
    }
};
} // namespace

bool CameraComponent::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}
bool CameraComponent::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (!p) {
        return false;
    }
    const auto name = p->GetName();
    if (name == "PostProcess") {
        auto ep = CreateProperty(path);
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new InterfacePtrEntityValue<IPostProcess>(ep, { GetInternalScene(), ClassId::PostProcess })));
    }
    if (name == "ColorTargetCustomization") {
        auto ep = CreateProperty(path);
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i && i->PushValue(META_NS::IValue::Ptr(new ConvertingArrayValue<FormatConverter>(ep)));
    }
    return false;
}

BASE_NS::string CameraComponent::GetName() const
{
    return "CameraComponent";
}
Future<bool> CameraComponent::SetActive(bool active)
{
    auto sf = SceneFlags();
    auto flags = META_NS::GetValue(sf);
    if (active) {
        flags |= uint32_t(CameraSceneFlag::ACTIVE_RENDER_BIT);
    } else {
        flags &= ~uint32_t(CameraSceneFlag::ACTIVE_RENDER_BIT | CameraSceneFlag::MAIN_CAMERA_BIT);
    }
    META_NS::SetValue(sf, flags);
    return SyncPropertyDirect(GetInternalScene(), sf);
}
Future<bool> CameraComponent::SetRenderTarget(const IRenderTarget::Ptr& target)
{
    auto scene = GetInternalScene();
    return scene->AddTaskOrRunDirectly([=] {
        bool res = UpdateCameraRenderTarget(object_, target);
        if (res) {
            if (target) {
                if (interface_cast<META_NS::IDynamicResource>(target)) {
                    scene->AddRenderingCamera(GetSelf<IInternalCamera>());
                }
            } else {
                scene->RemoveRenderingCamera(GetSelf<IInternalCamera>());
            }
            renderTarget_ = target;
        }
        return res;
    });
}
bool CameraComponent::IsActive() const
{
    return uint32_t(SceneFlags()->GetValue()) & uint32_t(CameraSceneFlag::ACTIVE_RENDER_BIT);
}
void CameraComponent::NotifyRenderTargetChanged()
{
    if (renderTarget_ && IsActive()) {
        auto scene = GetInternalScene();
        if (auto dr = interface_pointer_cast<META_NS::IDynamicResource>(renderTarget_)) {
            // anyone listening?
            if (dr->EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
                (void)scene->AddTaskOrRunDirectly(
                    [weak = META_NS::IDynamicResource::WeakPtr(dr)] {
                        if (auto d = weak.lock()) {
                            META_NS::Invoke<META_NS::IOnChanged>(d->OnResourceChanged());
                        }
                    },
                    scene->GetContext()->GetApplicationQueue());
            }
        }
    }
}

bool CameraComponent::Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext)
{
    if (target) {
        META_NS::IContainer::FindOptions options;
        options.behavior = META_NS::TraversalType::NO_HIERARCHY;
        options.uids = { IInputReceiver::UID };
        options.strict = false;
        inputReceivers_.SetTarget(target->GetAttachmentContainer(), options);
    }
    return true;
}

bool CameraComponent::Detaching(const IAttach::Ptr& target)
{
    inputReceivers_.Reset();
    return true;
}

void CameraComponent::SendInputEvent(PointerEvent& event)
{
    if (inputReceivers_.HasTarget() && !event.handled) {
        for (auto&& receiver : inputReceivers_.FindAll()) {
            receiver->OnInput(event);
            if (event.handled) {
                // Stop iteration if event.handled = true
                return;
            }
        }
    }
}

Future<NodeHits> CameraComponent::CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const
{
    auto scene = GetInternalScene();
    return scene->AddTaskOrRunDirectly([=] {
        RayCastOptions ops = options;
        if (ops.layerMask == NONE_LAYER_MASK) {
            ops.layerMask = CameraLayerMask()->GetValue();
        }
        NodeHits result;
        if (auto ir = interface_cast<IInternalRayCast>(scene)) {
            result = ir->CastRay(object_, pos, ops);
        }
        return result;
    });
}
Future<BASE_NS::Math::Vec3> CameraComponent::ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const
{
    auto scene = GetInternalScene();
    return scene->AddTaskOrRunDirectly([=] {
        BASE_NS::Math::Vec3 result;
        if (auto ir = interface_cast<IInternalRayCast>(scene)) {
            result = ir->ScreenPositionToWorld(object_, pos);
        }
        return result;
    });
}
Future<BASE_NS::Math::Vec3> CameraComponent::WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const
{
    auto scene = GetInternalScene();
    return scene->AddTaskOrRunDirectly([=] {
        BASE_NS::Math::Vec3 result;
        if (auto ir = interface_cast<IInternalRayCast>(scene)) {
            result = ir->WorldPositionToScreen(object_, pos);
        }
        return result;
    });
}

SCENE_END_NAMESPACE()
