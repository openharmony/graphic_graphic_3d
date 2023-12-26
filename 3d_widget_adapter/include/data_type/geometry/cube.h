/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
