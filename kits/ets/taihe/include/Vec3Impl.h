/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_VEC3_IMPL_H
#define OHOS_3D_VEC3_IMPL_H

#include "Vec3Proxy.h"

class Vec3Impl {
public:
    Vec3Impl(const std::shared_ptr<Vec3Proxy> proxy) : proxy_(proxy)
    {}

    Vec3Impl(const BASE_NS::Math::Vec3 &data) : stageData_(data)
    {}

    ~Vec3Impl()
    {
        proxy_.reset();
    }

    double getX()
    {
        if (proxy_) {
            return proxy_->GetX();
        }
        return stageData_.x;
    }

    void setX(double x)
    {
        if (proxy_) {
            proxy_->SetX(x);
        } else {
            stageData_.x = x;
        }
    }

    double getY()
    {
        if (proxy_) {
            return proxy_->GetY();
        }
        return stageData_.y;
    }

    void setY(double y)
    {
        if (proxy_) {
            proxy_->SetY(y);
        } else {
            stageData_.y = y;
        }
    }

    double getZ()
    {
        if (proxy_) {
            return proxy_->GetZ();
        }
        return stageData_.z;
    }

    void setZ(double z)
    {
        if (proxy_) {
            proxy_->SetZ(z);
        } else {
            stageData_.z = z;
        }
    }

private:
    std::shared_ptr<Vec3Proxy> proxy_{nullptr};
    // non meta property data
    BASE_NS::Math::Vec3 stageData_ = BASE_NS::Math::ZERO_VEC3;
};
#endif  // OHOS_3D_VEC3_IMPL_H