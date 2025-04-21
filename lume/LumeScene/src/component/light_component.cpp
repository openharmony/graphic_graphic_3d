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

#include "light_component.h"

#include "converting_value.h"

SCENE_BEGIN_NAMESPACE()
namespace {
struct ColorConverter {
    using SourceType = BASE_NS::Color;
    using TargetType = BASE_NS::Math::Vec3;

    static SourceType ConvertToSource(META_NS::IAny&, const TargetType& v)
    {
        return SourceType { v.x, v.y, v.z, 0.f };
    }
    static TargetType ConvertToTarget(const SourceType& v)
    {
        return TargetType { v.x, v.y, v.z };
    }
};
} // namespace
bool LightComponent::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "Color") {
        auto ep = object_->CreateEngineProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i && i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<ColorConverter>(ep)));
    }
    return false;
}
BASE_NS::string LightComponent::GetName() const
{
    return "LightComponent";
}

SCENE_END_NAMESPACE()