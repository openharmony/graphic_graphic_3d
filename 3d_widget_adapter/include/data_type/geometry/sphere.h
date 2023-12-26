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
