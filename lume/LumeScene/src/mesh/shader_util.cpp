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

#include "shader_util.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>

#include <3d/render/default_material_constants.h>
#include <render/device/intf_shader_manager.h>

SCENE_BEGIN_NAMESPACE()

bool ShaderUtil::Build(const META_NS::IMetadata::Ptr& d)
{
    IInternalScene::Ptr p;
    if (Super::Build(d)) {
        p = GetInterfaceBuildArg<IInternalScene>(d, "Scene");
        scene_ = p;
    }
    return p != nullptr;
}

Future<IShader::Ptr> ShaderUtil::CreateDefaultShader() const
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=] {
            auto& shaderMgr = scene->GetRenderContext().GetDevice().GetShaderManager();
            auto renderSlotId =
                shaderMgr.GetRenderSlotId(CORE3D_NS::DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
            auto rsd = shaderMgr.GetRenderSlotData(renderSlotId);

            auto shader = META_NS::GetObjectRegistry().Create<IShader>(ClassId::Shader);
            if (auto i = interface_cast<IRenderResource>(shader)) {
                i->SetRenderHandle(scene, rsd.shader);
            }
            return shader;
        });
    }
    return {};
}

SCENE_END_NAMESPACE()
