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

#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/metadata_util.h>

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
CORE_NS::IResource::Ptr ImageResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    auto context = context_.lock();
    if (s.payload && context) {
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
    }
    return res;
}
bool ImageResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        if (auto i = interface_cast<META_NS::IAttach>(p)) {
            if (auto c = i->GetAttachmentContainer(false)) {
                if (auto opts = c->FindAny<META_NS::IObjectResourceOptions>("", META_NS::TraversalType::NO_HIERARCHY)) {
                    opts->Save(*s.options, nullptr);
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