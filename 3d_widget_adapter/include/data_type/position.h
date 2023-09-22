/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_POSITION_H
#define OHOS_RENDER_3D_POSITION_H

#include "data_type/vec3.h"

namespace OHOS::Render3D {
class Position {
public:
    Position() = default;
    ~Position() = default;

    void SetPosition(const OHOS::Render3D::Vec3& vec)
    {
        pos_ = vec;
    }

    void SetDistance(const float distance)
    {
        distance_ = distance;
    }

    void SetIsAngular(bool isAngular)
    {
        isAngular_ = isAngular;
    }

    const Vec3& GetPosition() const
    {
        return pos_;
    }

    float GetX() const
    {
        return pos_.GetX();
    }

    float GetY() const
    {
        return pos_.GetY();
    }

    float GetZ() const
    {
        return pos_.GetZ();
    }

    float GetDistance() const
    {
        return distance_;
    }

    bool GetIsAngular() const
    {
        return isAngular_;
    }

private:
    OHOS::Render3D::Vec3 pos_ { 0.0f, 0.0f, 4.0f };
    float distance_ { 0.0f };
    bool isAngular_ = false;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_POSITION_H
