/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SPHERE_H
#define OHOS_RENDER_3D_SPHERE_H

#include "geometry.h"

namespace OHOS::Render3D {
class Sphere : public Geometry {
public:
    Sphere(std::string name, float radius, float rings, float sectors, Vec3& position)
        : Geometry(name, position), radius_(radius), rings_(rings), sectors_(sectors) {};
    virtual ~Sphere() {};

    int32_t GetType() const override
    {
        return GeometryType::SPHARE;
    }

    float GetRadius() const
    {
        return radius_;
    }

    float GetRings() const
    {
        return rings_;
    }

    float GetSectors() const
    {
        return sectors_;
    }
protected:
    bool IsEqual(const Geometry& obj) const override
    {
        const auto& m = reinterpret_cast<const Sphere&>(obj);

        return GetRadius() == m.GetRadius()
            && GetRings() == m.GetRings()
            && GetSectors() == m.GetSectors();
    }

private:
    float radius_;
    float rings_;
    float sectors_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SPHERE_H
