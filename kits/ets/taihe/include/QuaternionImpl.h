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

#ifndef OHOS_3D_QUATERNION_IMPL_H
#define OHOS_3D_QUATERNION_IMPL_H

#include "QuatProxy.h"

class QuaternionImpl {
public:
    QuaternionImpl(const std::shared_ptr<QuatProxy> proxy) : proxy_(proxy)
    {}

    QuaternionImpl(const BASE_NS::Math::Quat &quat) : stageData_(quat)
    {}

    ~QuaternionImpl()
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

    double getW()
    {
        if (proxy_) {
            return proxy_->GetW();
        }
        return stageData_.w;
    }

    void setW(double w)
    {
        if (proxy_) {
            proxy_->SetW(w);
        } else {
            stageData_.w = w;
        }
    }

private:
    std::shared_ptr<QuatProxy> proxy_{nullptr};
    // non meta property data
    BASE_NS::Math::Quat stageData_;
};
#endif  // OHOS_3D_QUATERNION_IMPL_H