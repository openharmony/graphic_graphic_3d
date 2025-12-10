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

#include "image_type.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <3d/ecs/components/render_handle_component.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/metadata_util.h>

#include "../../ecs_component/resource_component.h"

SCENE_BEGIN_NAMESPACE()

bool ImageResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct ImageResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string ImageResourceType::GetResourceName() const
{
    return "ImageResource";
}
BASE_NS::Uid ImageResourceType::GetResourceType() const
{
    return ClassId::ImageResource.Id().ToUid();
}

static META_NS::IObjectResourceOptions::Ptr CreateImageOptions(
    const IRenderContext::Ptr& context, const IImage::Ptr& image)
{
    META_NS::IObjectResourceOptions::Ptr opts;
    auto& gpuRes = context->GetRenderer()->GetDevice().GetGpuResourceManager();
    RENDER_NS::RenderHandleReference handle;
    if (auto i = interface_cast<IRenderResource>(image)) {
        handle = i->GetRenderHandle();
    }
    if (handle) {
        auto info = gpuRes.GetBufferDescriptor(handle);
        ImageLoadInfo flags;
        flags.info.usageFlags = static_cast<ImageUsageFlag>(info.usageFlags);
        flags.info.memoryFlags = static_cast<MemoryPropertyFlag>(info.memoryPropertyFlags);
        flags.info.creationFlags = static_cast<EngineImageCreationFlag>(info.engineCreationFlags);
        opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions);
        if (opts) {
            opts->SetProperty("ImageLoadInfo", flags);
        }
    }
    return opts;
}

static CORE_NS::IResource::Ptr LoadImageFromPath(
    const CORE_NS::IResourceType::StorageInfo& s, const IRenderContext::Ptr& context)
{
    CORE_NS::IResource::Ptr res;
    ImageLoadInfo flags = DEFAULT_IMAGE_LOAD_INFO;
    META_NS::IObjectResourceOptions::Ptr opts;
    if (s.options) {
        opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions);
        if (opts) {
            opts->Load(*s.options, nullptr, nullptr);
            if (auto p = opts->GetProperty<ImageLoadInfo>("ImageLoadInfo")) {
                flags = p->GetValue();
            }
        }
    }
    if (auto rman = META_NS::GetObjectRegistry().Create<IRenderResourceManager>(
            ClassId::RenderResourceManager, CreateRenderContextArg(context))) {
        res = interface_pointer_cast<CORE_NS::IResource>(rman->LoadImage(s.path, flags).GetResult());
        if (opts) {
            if (auto i = interface_cast<META_NS::IAttach>(res)) {
                i->Attach(opts);
            }
        }
    }
    return res;
}

static RENDER_NS::RenderHandleReference FindRenderHandle(const IInternalScene::Ptr is, const CORE_NS::ResourceId& id)
{
    if (auto resuMan =
            static_cast<IResourceComponentManager*>(is->GetEcsContext().FindComponent<ResourceComponent>())) {
        if (auto renderMan = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                is->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
            auto ent = resuMan->GetEntityWithResource(id);
            if (CORE_NS::EntityUtil::IsValid(ent)) {
                if (auto r = renderMan->Read(ent)) {
                    return r->reference;
                }
            }
        }
    }
    return {};
}

CORE_NS::IResource::Ptr ImageResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;

    // if path is empty, we expect this resource has been created from exiting image and the scene context
    // must be given to find it from the ecs.
    if (s.path.empty()) {
        if (auto scene = interface_pointer_cast<IScene>(s.context)) {
            auto is = scene->GetInternalScene();
            res = is->RunDirectlyOrInTask([&]() {
                CORE_NS::IResource::Ptr r;
                if (auto handle = FindRenderHandle(is, s.id)) {
                    if (auto img = interface_pointer_cast<IRenderResource>(is->CreateObject(ClassId::Image))) {
                        img->SetRenderHandle(handle);
                        r = interface_pointer_cast<CORE_NS::IResource>(img);
                    }
                } else {
                    CORE_LOG_W("Could not find render handle for resource: %s", s.id.ToString().c_str());
                }
                if (auto i = interface_cast<META_NS::IAttach>(r)) {
                    if (auto opts = CreateImageOptions(is->GetContext(), interface_pointer_cast<IImage>(r))) {
                        i->Attach(opts);
                    }
                }
                return r;
            });
        } else {
            CORE_LOG_W("Scene context not set for image when loading: %s", s.id.ToString().c_str());
        }
        if (!res) {
            return nullptr;
        }
    }

    auto context = context_.lock();
    if (s.payload && context) {
        res = LoadImageFromPath(s, context);
    }
    return res;
}
bool ImageResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        if (auto i = interface_cast<META_NS::IAttach>(p)) {
            if (auto c = i->GetAttachmentContainer(false)) {
                if (auto opts = c->FindAny<META_NS::IObjectResourceOptions>("", META_NS::TraversalType::NO_HIERARCHY)) {
                    opts->Save(*s.options, nullptr, nullptr);
                }
            }
        }
    }
    return true;
}
bool ImageResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const
{
    if (auto context = context_.lock()) {
        ImageLoadInfo flags = DEFAULT_IMAGE_LOAD_INFO;
        if (auto att = interface_cast<META_NS::IAttach>(res)) {
            if (auto cont = att->GetAttachmentContainer(false)) {
                if (auto opts =
                        cont->FindAny<META_NS::IObjectResourceOptions>("", META_NS::TraversalType::NO_HIERARCHY)) {
                    if (auto p = opts->GetProperty<ImageLoadInfo>("ImageLoadInfo")) {
                        flags = p->GetValue();
                    }
                }
            }
        }
        if (auto rman = META_NS::GetObjectRegistry().Create<IRenderResourceManager>(
                ClassId::RenderResourceManager, CreateRenderContextArg(context))) {
            auto reloaded = interface_pointer_cast<IRenderResource>(rman->LoadImage(s.path, flags).GetResult());
            if (reloaded) {
                if (auto i = interface_cast<IRenderResource>(res)) {
                    i->SetRenderHandle(reloaded->GetRenderHandle());
                    return true;
                }
            }
        }
    }
    return false;
}

SCENE_END_NAMESPACE()