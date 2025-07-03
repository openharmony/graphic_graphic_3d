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

#include "shader_type.h"

#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/metadata_util.h>

#include "serialization/util.h"

SCENE_BEGIN_NAMESPACE()

bool ShaderResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct ShaderResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string ShaderResourceType::GetResourceName() const
{
    return "ShaderResource";
}
BASE_NS::Uid ShaderResourceType::GetResourceType() const
{
    return ClassId::ShaderResource.Id().ToUid();
}

CORE_NS::IResource::Ptr ShaderResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (auto rman = META_NS::GetObjectRegistry().Create<IRenderResourceManager>(
            ClassId::RenderResourceManager, CreateRenderContextArg(context_.lock()))) {
        res = interface_pointer_cast<CORE_NS::IResource>(rman->LoadShader(s.path).GetResult());
    }
    if (res && s.options) {
        if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
                META_NS::ClassId::ObjectResourceOptions)) {
            opts->Load(*s.options, nullptr, nullptr);
            auto in = interface_cast<META_NS::IMetadata>(opts);
            auto out = interface_cast<META_NS::IMetadata>(res);
            if (in && out) {
                SerCopy(*in, *out);
            }
        }
    }
    return res;
}
bool ShaderResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
                META_NS::ClassId::ObjectResourceOptions)) {
            auto in = interface_cast<META_NS::IMetadata>(p);
            auto out = interface_cast<META_NS::IMetadata>(opts);
            if (auto i = interface_cast<META_NS::IDerivedFromResource>(p)) {
                opts->SetBaseResource(i->GetResource());
            }
            if (in && out) {
                SerCloneAllToDefaultIfSet(*in, *out);
                opts->Save(*s.options, nullptr);
            }
        }
    }
    return true;
}
bool ShaderResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& p) const
{
    if (auto context = context_.lock()) {
        if (auto dyn = interface_cast<META_NS::IDynamicResource>(p)) {
            context
                ->AddTask([&] {
                    auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
                    man.ReloadShaderFile(s.path);
                })
                .Wait();
            META_NS::Invoke<META_NS::IOnChanged>(dyn->OnResourceChanged());
            return true;
        }
    }
    return false;
}

SCENE_END_NAMESPACE()