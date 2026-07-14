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
#include <scene/interface/ecs/resource_component.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <3d/ecs/components/render_handle_component.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include <meta/api/util.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>

#include "../config.h"
#include "../objects/index.h"

SCENE_IMP_BEGIN_NAMESPACE()

static SCENE_NS::IRenderContext::Ptr GetRenderContext(const IImporter::WeakPtr& importer)
{
    auto imp = interface_pointer_cast<IImporterInternal>(importer.lock());
    if (!imp) {
        return nullptr;
    }
    return imp->GetConfig().context;
}

static SCENE_NS::ImageLoadInfo GetImageLoadInfo(const CORE_NS::IResourceOptions::Ptr& options)
{
    SCENE_NS::ImageLoadInfo flags = SCENE_NS::DEFAULT_IMAGE_LOAD_INFO;
    if (auto meta = interface_cast<META_NS::IMetadata>(options)) {
        flags = META_NS::GetValue<SCENE_NS::ImageLoadInfo>(
            meta->GetProperty("ImageLoadInfo"), SCENE_NS::DEFAULT_IMAGE_LOAD_INFO);
    }
    return flags;
}

static RENDER_NS::RenderHandleReference FindRenderHandle(
    const SCENE_NS::IInternalScene::Ptr& is, const CORE_NS::ResourceId& id)
{
    auto resuMan = static_cast<SCENE_NS::IResourceComponentManager*>(
        is->GetEcsContext().FindComponent<SCENE_NS::ResourceComponent>());
    auto renderMan = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
        is->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>());
    if (resuMan && renderMan) {
        auto ent = resuMan->GetEntity(id);
        if (CORE_NS::EntityUtil::IsValid(ent)) {
            if (auto r = renderMan->Read(ent)) {
                return r->reference;
            }
        }
    }
    return {};
}

static META_NS::IObject::Ptr CreateImageTemplate(const SCENE_NS::ImageLoadInfo& flags)
{
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::ImageTemplate);
    if (auto meta = interface_cast<META_NS::IMetadata>(obj)) {
        META_NS::SetValue(meta->GetProperty<SCENE_NS::ImageLoadInfo>("ImageLoadInfo"), flags);
    }
    return obj;
}

static void AttachImageTemplate(const META_NS::IObject::Ptr& tmpl, CORE_NS::IResource& image)
{
    if (!tmpl) {
        return;
    }
    if (auto att = interface_cast<META_NS::IAttach>(&image)) {
        att->Attach(tmpl);
    }
}

static SCENE_NS::ImageLoadInfo ReadGpuImageLoadInfo(
    const SCENE_NS::IRenderContext::Ptr& context, const CORE_NS::IResource::Ptr& image)
{
    SCENE_NS::ImageLoadInfo flags = SCENE_NS::DEFAULT_IMAGE_LOAD_INFO;
    auto renderResource = interface_cast<SCENE_NS::IRenderResource>(image);
    if (!context || !renderResource) {
        return flags;
    }
    auto handle = renderResource->GetRenderHandle();
    if (!handle) {
        return flags;
    }
    auto& gpuRes = context->GetRenderer()->GetDevice().GetGpuResourceManager();
    auto desc = gpuRes.GetImageDescriptor(handle);
    flags.info.usageFlags = static_cast<SCENE_NS::ImageUsageFlag>(desc.usageFlags);
    flags.info.memoryFlags = static_cast<SCENE_NS::MemoryPropertyFlag>(desc.memoryPropertyFlags);
    flags.info.creationFlags = static_cast<SCENE_NS::EngineImageCreationFlag>(desc.engineCreationFlags);
    return flags;
}

static CORE_NS::IResource::Ptr LoadImageFromScene(
    const CORE_NS::IResourceType::StorageInfo& s, const SCENE_NS::IScene::Ptr& scene)
{
    auto is = scene->GetInternalScene();
    return is->RunDirectlyOrInTask([&is, &s]() -> CORE_NS::IResource::Ptr {
        if (auto handle = FindRenderHandle(is, s.id)) {
            if (auto img =
                    interface_pointer_cast<SCENE_NS::IRenderResource>(is->CreateObject(SCENE_NS::ClassId::Image))) {
                img->SetRenderHandle(handle);
                auto res = interface_pointer_cast<CORE_NS::IResource>(img);
                AttachImageTemplate(CreateImageTemplate(ReadGpuImageLoadInfo(is->GetContext(), res)), *res);
                return res;
            }
        } else {
            CORE_LOG_E("Could not find render handle for resource: %s", s.id.ToString().c_str());
        }
        return nullptr;
    });
}

static CORE_NS::IResource::Ptr LoadImageFromPath(
    const CORE_NS::IResourceType::StorageInfo& s, const SCENE_NS::IRenderContext::Ptr& context)
{
    auto flags = GetImageLoadInfo(s.options);
    if (auto rman = META_NS::GetObjectRegistry().Create<SCENE_NS::IRenderResourceManager>(
            SCENE_NS::ClassId::RenderResourceManager, SCENE_NS::CreateRenderContextArg(context))) {
        if (SCENE_NS::IsInImporterDeferredScope(context)) {
            return interface_pointer_cast<CORE_NS::IResource>(rman->LoadImageDeferred(s.path, flags));
        }
        return interface_pointer_cast<CORE_NS::IResource>(rman->LoadImage(s.path, flags).GetResult());
    }
    return nullptr;
}

// Copy the optional INamed "Name" from the load options onto the freshly
// loaded image. ImageTemplate::ApplyOptions is a no-op, so the importer's name
// has to be transferred to the live resource here.
static void ApplyResourceName(const CORE_NS::IResourceOptions::Ptr& options, CORE_NS::IResource& image)
{
    auto src = interface_cast<META_NS::INamed>(options);
    auto dst = interface_cast<META_NS::INamed>(&image);
    if (src && dst) {
        if (auto name = META_NS::GetValue(src->Name()); !name.empty()) {
            META_NS::SetValue(dst->Name(), name);
        }
    }
}

CORE_NS::IResource::Ptr ImageResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr image;
    if (s.path.empty()) {
        auto rcontext = interface_cast<SCENE_NS::IResourceContext>(s.context);
        auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<SCENE_NS::IScene>(s.context);
        if (!scene) {
            CORE_LOG_E("Scene context not set for image when loading: %s", s.id.ToString().c_str());
            return nullptr;
        }
        image = LoadImageFromScene(s, scene);
    } else {
        auto context = GetRenderContext(importer_);
        if (!context) {
            CORE_LOG_E("No render context when loading image resource: %s (path: %s)",
                s.id.ToString().c_str(),
                BASE_NS::string(s.path).c_str());
            return nullptr;
        }
        image = LoadImageFromPath(s, context);
        if (image && s.options) {
            AttachImageTemplate(interface_pointer_cast<META_NS::IObject>(s.options), *image);
        }
    }
    if (image) {
        ApplyResourceName(s.options, *image);
    }
    return image;
}

bool ImageResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const
{
    if (s.path.empty()) {
        return false;
    }
    auto context = GetRenderContext(importer_);
    if (!context) {
        CORE_LOG_E("No render context when reloading image resource: %s", s.id.ToString().c_str());
        return false;
    }
    auto flags = GetImageLoadInfo(s.options);
    if (auto rman = META_NS::GetObjectRegistry().Create<SCENE_NS::IRenderResourceManager>(
            SCENE_NS::ClassId::RenderResourceManager, SCENE_NS::CreateRenderContextArg(context))) {
        auto reloaded = interface_pointer_cast<SCENE_NS::IRenderResource>(rman->LoadImage(s.path, flags).GetResult());
        if (reloaded) {
            if (auto i = interface_cast<SCENE_NS::IRenderResource>(res)) {
                i->SetRenderHandle(reloaded->GetRenderHandle());
                return true;
            }
        }
    }
    return false;
}

SCENE_IMP_END_NAMESPACE()
