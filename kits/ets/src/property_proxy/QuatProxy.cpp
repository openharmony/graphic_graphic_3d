/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "property_proxy/QuatProxy.h"

namespace OHOS::Render3D {
QuatProxy::QuatProxy(const META_NS::Property<BASE_NS::Math::Quat> &prop) : PropertyProxy<BASE_NS::Math::Quat>(prop)
{}

QuatProxy::~QuatProxy()
{}

float QuatProxy::GetX() const
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    return value.x;
}

void QuatProxy::SetX(const float x)
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    if (value.x == x) {
        return;
    }
    value.x = x;
    META_NS::SetValue(prop, value);
}

float QuatProxy::GetY() const
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    return value.y;
}

void QuatProxy::SetY(const float y)
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    if (value.y == y) {
        return;
    }
    value.y = y;
    META_NS::SetValue(prop, value);
}

float QuatProxy::GetZ() const
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    return value.z;
}

void QuatProxy::SetZ(const float z)
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    if (value.z == z) {
        return;
    }
    value.z = z;
    META_NS::SetValue(prop, value);
}

float QuatProxy::GetW() const
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    return value.w;
}

void QuatProxy::SetW(const float w)
{
    META_NS::Property<BASE_NS::Math::Quat> prop(prop_);
    BASE_NS::Math::Quat value = META_NS::GetValue(prop);
    if (value.w == w) {
        return;
    }
    value.w = w;
    META_NS::SetValue(prop, value);
}
}  // namespace OHOS::Render3D