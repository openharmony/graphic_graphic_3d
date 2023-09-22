/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
