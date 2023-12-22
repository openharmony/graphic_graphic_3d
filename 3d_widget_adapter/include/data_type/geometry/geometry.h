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
