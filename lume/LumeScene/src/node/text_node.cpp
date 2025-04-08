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

#include "text_node.h"

#include <scene/interface/intf_material.h>

#include "core/ecs.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity TextNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto ecs = scene->GetEcsContext().GetNativeEcs();
    auto textManager = CORE_NS::GetManager<TEXT3D_NS::ITextComponentManager>(*ecs);
    auto materialManager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    if (!textManager || !materialManager) {
        return {};
    }
    auto entity = ecs->GetEntityManager().Create();
    scene->GetEcsContext().AddDefaultComponents(entity);
    textManager->Create(entity);
    materialManager->Create(entity);
    return entity;
}

bool TextNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<IText>();
        if (!att.empty()) {
            return true;
        }
        if (auto att = GetSelf<META_NS::IAttach>()) {
            if (auto c = att->GetAttachmentContainer(true)->FindByName<IEcsObjectAccess>("MaterialComponent")) {
                auto mat = META_NS::GetObjectRegistry().Create<IMaterial>(ClassId::Material);
                auto matatt = interface_cast<META_NS::IAttach>(mat);
                auto matacc = interface_cast<IEcsObjectAccess>(mat);
                if (matatt && matacc) {
                    matatt->Attach(c);
                    if (matacc->SetEcsObject(c->GetEcsObject())) {
                        fontColor_ = mat->Textures()->GetValueAt(0);
                    }
                }
            }
        }
    }
    return fontColor_ != nullptr;
}

SCENE_END_NAMESPACE()
