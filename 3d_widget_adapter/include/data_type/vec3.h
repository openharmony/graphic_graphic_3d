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

#ifndef OHOS_RENDER_3D_VEC3_H
#define OHOS_RENDER_3D_VEC3_H

#include <string>
#include "data_type/constants.h"

namespace OHOS::Render3D {
class Vec3 {
public:
    Vec3() {}
    Vec3(float x, float y, float z) : x_(x), y_(y), z_(z) {}
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetZ() const { return z_; }
    void SetX(float x) { x_ = x; }
    void SetY(float y) { y_ = y; }
    void SetZ(float z) { z_ = z; }

private:
    union {
        struct {
            float x_ = 0.0f;
            float y_ = 0.0f;
            float z_ = 0.0f;
        };
        float data_[3];
    };
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_VEC3_H
