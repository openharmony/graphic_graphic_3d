/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_GEOMETRY_H
#define OHOS_RENDER_3D_GEOMETRY_H

#include "../vec3.h"
#include <string>

namespace OHOS::Render3D {
class Geometry {
public:
    Geometry(std::string name, Vec3& position, bool castShadows = false, bool receiveShadows = false) : name_(name),
        position_(position), castShadows_(castShadows), receiveShadows_(receiveShadows) {};
    ~Geometry() = default;

    virtual int32_t GetType() const = 0;
    virtual bool Equals(const Geometry& obj) const
    {
        return GetType() == obj.GetType()
            && GetName() == obj.GetName()
            && CastShadows() == obj.CastShadows()
            && ReceiveShadows() == obj.ReceiveShadows()
            && IsEqual(obj);
    }

    bool PositionEquals(const Geometry& obj)
    {
        return GetPosition().GetX() == obj.GetPosition().GetX()
            && GetPosition().GetY() == obj.GetPosition().GetY()
            && GetPosition().GetZ() == obj.GetPosition().GetZ();
    }

    std::string GetName() const
    {
        return name_;
    }

    const Vec3& GetPosition() const
    {
        return position_;
    }

    bool CastShadows() const
    {
        return castShadows_;
    }

    bool ReceiveShadows() const
    {
        return receiveShadows_;
    }

protected:
    virtual bool IsEqual(const Geometry& obj) const = 0;

private:
    std::string name_;
    Vec3 position_;
    bool castShadows_ { false };
    bool receiveShadows_ { false };
};

enum GeometryType {
    CUBE,
    SPHARE,
    CONE,
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GEOMETRY_H
