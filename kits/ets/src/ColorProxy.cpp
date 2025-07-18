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

#include "ColorProxy.h"

ColorProxy::ColorProxy(const META_NS::Property<BASE_NS::Color> &prop) : PropertyProxy<BASE_NS::Color>(prop)
{}

ColorProxy::~ColorProxy()
{}

float ColorProxy::GetR() const
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    return value.r;
}

void ColorProxy::SetR(const float r)
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    if (value.r == r) {
        return;
    }
    value.r = r;
    META_NS::SetValue(prop, value);
}

float ColorProxy::GetG() const
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    return value.g;
}

void ColorProxy::SetG(const float g)
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    if (value.g == g) {
        return;
    }
    value.g = g;
    META_NS::SetValue(prop, value);
}

float ColorProxy::GetB() const
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    return value.b;
}

void ColorProxy::SetB(const float b)
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    if (value.b == b) {
        return;
    }
    value.b = b;
    META_NS::SetValue(prop, value);
}

float ColorProxy::GetA() const
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    return value.a;
}

void ColorProxy::SetA(const float a)
{
    META_NS::Property<BASE_NS::Color> prop(prop_);
    BASE_NS::Color value = META_NS::GetValue(prop);
    if (value.a == a) {
        return;
    }
    value.a = a;
    META_NS::SetValue(prop, value);
}