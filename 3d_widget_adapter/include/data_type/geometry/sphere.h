/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SV_SPHERE_H
#define OHOS_RENDER_3D_SV_SPHERE_H

#include "geometry.h"

namespace OHOS::Render3D {
class SVSphere : public SVGeometry {
    DECLARE_ACE_TYPE(SVSphere, SVGeometry)
public:
    SVSphere(std::string name, float radius, float rings, float sectors, Vec3& position)
        : SVGeometry(name, position), radius_(radius), rings_(rings), sectors_(sectors) {};
    ~SVSphere() = default;

    int32_t GetType() override
    {
        return 1; // SVSphere type id
    }

    float GetRadius()
    {
        return radius_;
    }

    float GetRings()
    {
        return rings_;
    }

    float GetSectors()
    {
        return sectors_;
    }

private:
    float radius_;
    float rings_;
    float sectors_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SV_SPHERE_H
