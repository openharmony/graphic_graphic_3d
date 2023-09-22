/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
