/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_CUBE_H
#define OHOS_RENDER_3D_CUBE_H

#include "geometry.h"

namespace OHOS::Render3D {
class Cube : public Geometry {
public:
    Cube(std::string name, float width, float height, float depth, Vec3& position) : Geometry(name, position),
        width_(width), height_(height), depth_(depth) {};
    virtual ~Cube() {};

    int32_t GetType() const override
    {
        return GeometryType::CUBE;
    }

    float GetWidth() const
    {
        return width_;
    }

    float GetHeight() const
    {
        return height_;
    }

    float GetDepth() const
    {
        return depth_;
    }
protected:
    bool IsEqual(const Geometry& obj) const override
    {
        const auto& m = reinterpret_cast<const Cube&>(obj);

        return GetWidth() == m.GetWidth()
            && GetHeight() == m.GetHeight()
            && GetDepth() == m.GetDepth();
    }
private:
    float width_ { 0.0f };
    float height_ { 0.0f };
    float depth_ { 0.0f };
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_CUBE_H
