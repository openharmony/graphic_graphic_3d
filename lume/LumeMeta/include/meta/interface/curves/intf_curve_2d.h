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
#ifndef META_INTERFACE_ICURVE_2D_H
#define META_INTERFACE_ICURVE_2D_H

#include <base/math/vector.h>

#include <meta/base/namespace.h>
#include <meta/interface/curves/intf_curve.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICurve2D, "1faeb3a5-54bc-461f-aaf0-7c5097167ba1")

/**
 * @brief ICurve is the base interface for all curves which transform a
 *        2D curve along an axis.
 */
class ICurve2D : public ICurve<BASE_NS::Math::Vec2> {
    META_INTERFACE(ICurve<BASE_NS::Math::Vec2>, ICurve2D)
public:
    /**
     * @brief The transformation function for this curve.
     * @param t The point along the curve where the transformation should happen.
     * @return Transformed value.
     */
    virtual BASE_NS::Math::Vec2 Transform(float t) const override = 0;
};

META_END_NAMESPACE()

#endif
