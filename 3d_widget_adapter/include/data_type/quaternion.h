/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_QUATERNION_H
#define OHOS_RENDER_3D_QUATERNION_H

#include <string>
#include "data_type/constants.h"

namespace OHOS::Render3D {
class Quaternion {
public:
    Quaternion() {}
    Quaternion(float x, float y, float z, float w) : x_(x), y_(y), z_(z), w_(w) {}
    float GetX() const
    {
        return x_;
    }
    float GetY() const
    {
        return y_;
    }
    float GetZ() const
    {
        return z_;
    }
    float GetW() const
    {
        return w_;
    }
    void SetX(float x)
    {
        x_ = x;
    }
    void SetY(float y)
    {
        y_ = y;
    }
    void SetZ(float z)
    {
        z_ = z;
    }
    void SetW(float w)
    {
        w_ = w;
    }
    bool operator==(const Quaternion& other) const
    {
        return (x_ == other.x_) && (y_ == other.y_) && (z_ == other.z_) && (w_ == other.w_);
    }

private:
    union {
        struct {
            float x_;
            float y_;
            float z_;
            float w_;
        };
        float data_[4] { 0.0f, 0.0f, 0.0f, 0.0f } ;
    };
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_QUATERNION_H
