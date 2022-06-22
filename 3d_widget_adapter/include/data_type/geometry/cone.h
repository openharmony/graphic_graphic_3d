/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SV_CONE_H
#define OHOS_RENDER_3D_SV_CONE_H

#include "geometry.h"

namespace OHOS::Render3D {
class SVCone : public SVGeometry {
    DECLARE_ACE_TYPE(SVCone, SVGeometry)
public:
    SVCone(std::string name, float radius, float length, float sectors, Vec3& position) :
        SVGeometry(name, position), radius_(radius), length_(length), sectors_(sectors){};
    ~SVCone() = default;

    int32_t GetType() override
    {
        return 2; // SVCone type id
    }

    float GetRadius()
    {
        return radius_;
    }

    float GetLength()
    {
        return length_;
    }

    float GetSectors()
    {
        return sectors_;
    }

private:
    float radius_;
    float length_;
    float sectors_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SV_CONE_H
