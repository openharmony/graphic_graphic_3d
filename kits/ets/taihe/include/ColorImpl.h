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

#ifndef OHOS_3D_COLOR_IMPL_H
#define OHOS_3D_COLOR_IMPL_H

#include "property_proxy/ColorProxy.h"
#include "property_proxy/Vec4Proxy.h"

namespace OHOS::Render3D::KITETS {
class ColorImpl {
public:
    ColorImpl(const std::shared_ptr<ColorProxy> proxy) : cproxy_(proxy)
    {}

    ColorImpl(const std::shared_ptr<Vec4Proxy> proxy) : vproxy_(proxy)
    {}

    ColorImpl(const BASE_NS::Color &color) : stageData_(color)
    {}

    ~ColorImpl()
    {
        cproxy_.reset();
        vproxy_.reset();
    }

    double getR()
    {
        if (cproxy_) {
            return cproxy_->GetR();
        } else if (vproxy_) {
            return vproxy_->GetX();
        } else {
            return stageData_.r;
        }
    }

    void setR(double r)
    {
        if (cproxy_) {
            cproxy_->SetR(r);
        } else if (vproxy_) {
            vproxy_->SetX(r);
        } else {
            stageData_.r = r;
        }
    }

    double getG()
    {
        if (cproxy_) {
            return cproxy_->GetG();
        } else if (vproxy_) {
            return vproxy_->GetY();
        } else {
            return stageData_.g;
        }
    }

    void setG(double g)
    {
        if (cproxy_) {
            cproxy_->SetG(g);
        } else if (vproxy_) {
            vproxy_->SetY(g);
        } else {
            stageData_.g = g;
        }
    }

    double getB()
    {
        if (cproxy_) {
            return cproxy_->GetB();
        } else if (vproxy_) {
            return vproxy_->GetZ();
        } else {
            return stageData_.b;
        }
    }

    void setB(double b)
    {
        if (cproxy_) {
            cproxy_->SetB(b);
        } else if (vproxy_) {
            vproxy_->SetZ(b);
        } else {
            stageData_.b = b;
        }
    }

    double getA()
    {
        if (cproxy_) {
            return cproxy_->GetA();
        } else if (vproxy_) {
            return vproxy_->GetW();
        } else {
            return stageData_.a;
        }
    }

    void setA(double a)
    {
        if (cproxy_) {
            cproxy_->SetA(a);
        } else if (vproxy_) {
            vproxy_->SetW(a);
        } else {
            stageData_.a = a;
        }
    }

private:
    std::shared_ptr<ColorProxy> cproxy_{nullptr};
    // To handle the camera's ClearColor
    std::shared_ptr<Vec4Proxy> vproxy_{nullptr};
    // non meta property data
    BASE_NS::Color stageData_ = BASE_NS::BLACK_COLOR;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_COLOR_IMPL_H