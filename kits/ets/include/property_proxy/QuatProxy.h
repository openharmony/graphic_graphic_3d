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

#ifndef OHOS_3D_QUAT_PROXY_H
#define OHOS_3D_QUAT_PROXY_H

#include <base/math/quaternion.h>

#include "property_proxy/PropertyProxy.h"

namespace OHOS::Render3D {
class QuatProxy : public PropertyProxy<BASE_NS::Math::Quat> {
public:
    explicit QuatProxy(const META_NS::Property<BASE_NS::Math::Quat> &prop);
    ~QuatProxy() override;

    float GetX() const;
    void SetX(const float x);

    float GetY() const;
    void SetY(const float y);

    float GetZ() const;
    void SetZ(const float z);

    float GetW() const;
    void SetW(const float w);
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_QUAT_PROXY_H