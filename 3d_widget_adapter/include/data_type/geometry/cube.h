/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SV_CUBE_H
#define OHOS_RENDER_3D_SV_CUBE_H

#include "geometry.h"

namespace OHOS::Render3D {
class SVCube : public SVGeometry {
    DECLARE_ACE_TYPE(SVCube, SVGeometry)
public:
    SVCube(std::string name, float width, float height, float depth, Vec3& position) :
        SVGeometry(name, position),  width_(width), height_(height), depth_(depth) {};
    ~SVCube() = default;

    int32_t GetType() override
    {
        return 0; // SVCube type id
    }

    float GetWidth()
    {
        return width_;
    }

    float GetHeight()
    {
        return height_;
    }

    float GetDepth()
    {
        return depth_;
    }

private:
    float width_;
    float height_;
    float depth_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SV_CUBE_H
