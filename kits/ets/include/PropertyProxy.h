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

#ifndef OHOS_3D_PROPERTY_PROXY_H
#define OHOS_3D_PROPERTY_PROXY_H

#include <base/math/vector.h>
#include <base/math/quaternion.h>
#include <base/util/color.h>

#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>
#include <meta/api/util.h>

template <typename Type>
class PropertyProxy {
public:
    explicit PropertyProxy(const META_NS::Property<Type> &prop) : prop_(prop.GetProperty())
    {}

    virtual ~PropertyProxy()
    {
        prop_.reset();
    }

    void SetValue(const Type &value)
    {
        META_NS::Property<Type>(prop_)->SetValue(value);
    }

    Type GetValue()
    {
        META_NS::Property<Type>(prop_)->GetValue();
    }

protected:
    META_NS::IProperty::Ptr prop_;
};
#endif  // OHOS_3D_PROPERTY_PROXY_H