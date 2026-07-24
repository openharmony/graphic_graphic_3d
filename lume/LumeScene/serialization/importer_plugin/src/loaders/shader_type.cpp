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

#include <scene/interface/resource/intf_render_resource_manager.h>

#include <render/device/intf_device.h>

#include <meta/interface/intf_event.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

#include "../config.h"

SCENE_IMP_BEGIN_NAMESPACE()

static SCENE_NS::IRenderContext::Ptr GetRenderContext(const IImporter::WeakPtr& importer)
{
    auto imp = interface_pointer_cast<IImporterInternal>(importer.lock());
    if (!imp) {
        return nullptr;
    }
    return imp->GetConfig().context;
}

CORE_NS::IResource::Ptr ShaderResourceType::LoadResource(const StorageInfo& s) const
{
    auto context = GetRenderContext(importer_);
    if (!context) {
        CORE_LOG_E("No render context when loading shader resource: %s (path: %s)",
            s.id.ToString().c_str(),
            BASE_NS::string(s.path).c_str());
        return nullptr;
    }
    CORE_NS::IResource::Ptr res;
    if (auto rman = META_NS::GetObjectRegistry().Create<SCENE_NS::IRenderResourceManager>(
            SCENE_NS::ClassId::RenderResourceManager, SCENE_NS::CreateRenderContextArg(context))) {
        res = interface_pointer_cast<CORE_NS::IResource>(rman->LoadShader(s.path).GetResult());
    }
    if (res && s.options) {
        s.options->ApplyOptions(*res, s.context);
    }
    return res;
}

bool ShaderResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const
{
    // no payload -> only refresh options
    if (res && s.options && !s.payload) {
        return s.options->ApplyOptions(*res, s.context);
    }
    auto context = GetRenderContext(importer_);
    if (!context) {
        CORE_LOG_E("No render context when reloading shader resource: %s", s.id.ToString().c_str());
        return false;
    }
    if (auto dyn = interface_cast<META_NS::IDynamicResource>(res)) {
        context->RunDirectlyOrInTask([&context, &s] {
            auto renderer = context->GetRenderer();
            if (!renderer) {
                CORE_LOG_E("Renderer is null when reloading shader resource: %s", s.id.ToString().c_str());
                return;
            }
            auto& man = renderer->GetDevice().GetShaderManager();
            man.ReloadShaderFile(s.path);
        });
        META_NS::Invoke<META_NS::IOnChanged>(dyn->OnResourceChanged());
        return true;
    }
    return false;
}

SCENE_IMP_END_NAMESPACE()
