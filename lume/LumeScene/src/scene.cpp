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

#include "scene.h"

#include <scene/ext/util.h>

#include <render/intf_render_context.h>

#include "component_factory.h"
#include "core/internal_scene.h"
#include "core/log.h"
#include "render_configuration.h"

SCENE_BEGIN_NAMESPACE()

SceneObject::~SceneObject()
{
    if (internal_) {
        if (updateTask_) {
            internal_->GetEngineTaskQueue()->CancelTask(updateTask_);
        }
        internal_->AddTask([&] { return internal_->Uninitialize(); }).Wait();
    }
}

bool SceneObject::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc =
            GetInterfaceBuildArg<RENDER_NS::IRenderContext>(d, "RenderContext");
        META_NS::ITaskQueue::Ptr engine = GetInterfaceBuildArg<META_NS::ITaskQueue>(d, "EngineQueue");
        META_NS::ITaskQueue::Ptr app = GetInterfaceBuildArg<META_NS::ITaskQueue>(d, "AppQueue");
        if (!engine || !app || !rc) {
            CORE_LOG_E("Invalid parameters to construct Scene");
            return false;
        }
        auto in = CreateShared<InternalScene>(
            GetSelf<IScene>(), BASE_NS::move(engine), BASE_NS::move(app), BASE_NS::move(rc));
        internal_ = in;
        in->SetSelf(internal_);
        AddBuiltinComponentFactories(internal_);
        auto customSystemGraphUri = d->GetProperty<BASE_NS::string>("customSystemGraphUri");
        if (customSystemGraphUri) {
            internal_->SetSystemGraphUri(customSystemGraphUri->GetValue());
            CORE_LOG_E("customSystemGraphUri %s", customSystemGraphUri->GetValue().c_str());
        }
        res = internal_->Initialize();
    }
    return res;
}

bool SceneObject::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "RenderConfiguration") {
        auto obj = CreateObject<IRenderConfiguration>(ClassId::RenderConfiguration).GetResult();
        if (META_NS::Property<IRenderConfiguration::Ptr> t { p }) {
            return t->SetValue(obj);
        }
    }
    return false;
}

bool SceneObject::AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    return false;
}

void SceneObject::StartAutoUpdate(META_NS::TimeSpan interval)
{
    if (updateTask_) {
        internal_->GetEngineTaskQueue()->CancelTask(updateTask_);
    }
    updateTask_ = internal_->GetEngineTaskQueue()->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([&] {
        internal_->Update();
        return true;
    }),
        interval);
}

Future<INode::Ptr> SceneObject::GetRootNode() const
{
    return internal_->AddTask([=] { return internal_->FindNode("", {}); });
}

Future<INode::Ptr> SceneObject::CreateNode(const BASE_NS::string_view uri, META_NS::ObjectId id)
{
    return internal_->AddTask([=, path = BASE_NS::string(uri)] { return internal_->CreateNode(path, id); });
}

Future<INode::Ptr> SceneObject::FindNode(BASE_NS::string_view uri, META_NS::ObjectId id) const
{
    return internal_->AddTask([=, path = BASE_NS::string(uri)] { return internal_->FindNode(path, id); });
}

Future<META_NS::IObject::Ptr> SceneObject::CreateObject(META_NS::ObjectId id)
{
    return internal_->AddTask([=] { return internal_->CreateObject(id); });
}

Future<bool> SceneObject::ReleaseNode(INode::Ptr&& node, bool recursive)
{
    return internal_->AddTask(
        [=, n = BASE_NS::move(node)]() mutable { return internal_->ReleaseNode(BASE_NS::move(n), recursive); });
}
Future<bool> SceneObject::RemoveNode(const INode::Ptr& node)
{
    return internal_->AddTask([=] { return internal_->RemoveNode(node); });
}

Future<BASE_NS::vector<ICamera::Ptr>> SceneObject::GetCameras() const
{
    return internal_->AddTask([=] { return internal_->GetCameras(); });
}

Future<BASE_NS::vector<META_NS::IAnimation::Ptr>> SceneObject::GetAnimations() const
{
    return internal_->AddTask([=] { return internal_->GetAnimations(); });
}

IInternalScene::Ptr SceneObject::GetInternalScene() const
{
    return internal_;
}

Future<bool> SceneObject::SetRenderMode(RenderMode mode)
{
    return internal_->AddTask([=] { return internal_->SetRenderMode(mode); });
}
Future<RenderMode> SceneObject::GetRenderMode() const
{
    return internal_->AddTask([=] { return internal_->GetRenderMode(); });
}
Future<NodeHits> SceneObject::CastRay(
    const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const
{
    return internal_->AddTask([=] {
        RayCastOptions ops;
        if (ops.layerMask == NONE_LAYER_MASK) {
            ops.layerMask = ALL_LAYER_MASK;
        }
        NodeHits result;
        if (auto ir = interface_cast<IInternalRayCast>(internal_)) {
            result = ir->CastRay(pos, dir, ops);
        }
        return result;
    });
}
SCENE_END_NAMESPACE()