/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_INTERFACE_IEASING_CURVE_H
#define META_INTERFACE_IEASING_CURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/curves/intf_curve_1d.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEasingCurve, "a27e0158-28c6-4e72-8ea2-a43301736653")

/**
 * @brief IEasingCurve is the interface for all 1D easing curves, which
 *        require the transform input to be between [0,1].
 */
class IEasingCurve : public ICurve1D {
    META_INTERFACE(ICurve1D, IEasingCurve)
public:
    /**
     * @brief The transformation function for this curve.
     * @param t The point along the curve where the transformation should happen.
     *          For easing curves, t should always be between [0,1].
     * @return Transformed value. For t=0 the function should return 0, and for t=1
     *         it should return 1.
     */
    float Transform(float t) const override = 0;
};

META_END_NAMESPACE()

#endif // API_UI_EASING_CURVE_H
