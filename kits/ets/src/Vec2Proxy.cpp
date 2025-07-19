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

#include "Vec2Proxy.h"

Vec2Proxy::Vec2Proxy(const META_NS::Property<BASE_NS::Math::Vec2> &prop) : PropertyProxy<BASE_NS::Math::Vec2>(prop)
{}

Vec2Proxy::~Vec2Proxy()
{}

float Vec2Proxy::GetX() const
{
    META_NS::Property<BASE_NS::Math::Vec2> prop(prop_);
    BASE_NS::Math::Vec2 value = META_NS::GetValue(prop);
    return value.x;
}

void Vec2Proxy::SetX(const float x)
{
    META_NS::Property<BASE_NS::Math::Vec2> prop(prop_);
    BASE_NS::Math::Vec2 value = META_NS::GetValue(prop);
    if (value.x == x) {
        return;
    }
    value.x = x;
    META_NS::SetValue(prop, value);
}

float Vec2Proxy::GetY() const
{
    META_NS::Property<BASE_NS::Math::Vec2> prop(prop_);
    BASE_NS::Math::Vec2 value = META_NS::GetValue(prop);
    return value.y;
}

void Vec2Proxy::SetY(const float y)
{
    META_NS::Property<BASE_NS::Math::Vec2> prop(prop_);
    BASE_NS::Math::Vec2 value = META_NS::GetValue(prop);
    if (value.y == y) {
        return;
    }
    value.y = y;
    META_NS::SetValue(prop, value);
}