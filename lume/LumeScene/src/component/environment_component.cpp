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

#include "environment_component.h"

#include "../entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string EnvironmentComponent::GetName() const
{
    return "EnvironmentComponent";
}

bool EnvironmentComponent::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "RadianceImage" || p->GetName() == "EnvironmentImage") {
        auto ep = object_->CreateProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(
                   META_NS::IValue::Ptr(new RenderResourceValue<IImage>(ep, { object_->GetScene(), ClassId::Image })));
    }
    return AttachEngineProperty(p, path);
}

SCENE_END_NAMESPACE()
