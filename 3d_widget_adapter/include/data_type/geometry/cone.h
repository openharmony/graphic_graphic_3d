/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_CONE_H
#define OHOS_RENDER_3D_CONE_H

#include "geometry.h"

namespace OHOS::Render3D {
class Cone : public Geometry {
public:
    Cone(std::string name, float radius, float length, float sectors, Vec3& position) : Geometry(name, position),
        radius_(radius), length_(length), sectors_(sectors) {};
    virtual ~Cone() {};

    int32_t GetType() const override
    {
        return GeometryType::CONE; // Cone type id
    }

    float GetRadius() const
    {
        return radius_;
    }

    float GetLength() const
    {
        return length_;
    }

    float GetSectors() const
    {
        return sectors_;
    }
protected:
    bool IsEqual(const Geometry& obj) const override
    {
        const auto& m = reinterpret_cast<const Cone&>(obj);

        return GetRadius() == m.GetRadius()
            && GetLength() == m.GetLength()
            && GetSectors() == m.GetSectors();
    }
private:
    float radius_ { 0.0f };
    float length_ { 0.0f };
    float sectors_ { 0.0f };
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_CONE_H
