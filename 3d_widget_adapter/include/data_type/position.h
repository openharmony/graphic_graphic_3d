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

#ifndef OHOS_RENDER_3D_POSITION_H
#define OHOS_RENDER_3D_POSITION_H

#include "data_type/vec3.h"

namespace OHOS::Render3D {
class Position {
public:
    Position() = default;
    ~Position() = default;

    void SetPosition(const OHOS::Render3D::Vec3& vec)
    {
        pos_ = vec;
    }

    void SetDistance(const float distance)
    {
        distance_ = distance;
    }

    void SetIsAngular(bool isAngular)
    {
        isAngular_ = isAngular;
    }

    const Vec3& GetPosition() const
    {
        return pos_;
    }

    float GetX() const
    {
        return pos_.GetX();
    }

    float GetY() const
    {
        return pos_.GetY();
    }

    float GetZ() const
    {
        return pos_.GetZ();
    }

    float GetDistance() const
    {
        return distance_;
    }

    bool GetIsAngular() const
    {
        return isAngular_;
    }

private:
    OHOS::Render3D::Vec3 pos_ { 0.0f, 0.0f, 4.0f };
    float distance_ { 0.0f };
    bool isAngular_ = false;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_POSITION_H
