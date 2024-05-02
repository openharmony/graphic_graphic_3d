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

#ifndef META_INTERFACE_IBEZIER_H
#define META_INTERFACE_IBEZIER_H

#include <base/math/vector.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICubicBezier, "e7144ea4-2abc-48d3-9302-f2ff83cdb3c5")

/**
 * @brief Interface for a cubic bezier function which interpolates between [0,1] using
 *        ControlPoint1 and ControlPoint2 as the control points for the curve.
 *        The start point is implicitly [0,0] and the end point is implicitly [1,1].
 */
class ICubicBezier : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICubicBezier)
public:
    /**
     * @brief First control point. X coordinate must be within [0,1].
     */
    META_PROPERTY(BASE_NS::Math::Vec2, ControlPoint1)
    /**
     * @brief Second control point. X coordinate must be within [0,1].
     */
    META_PROPERTY(BASE_NS::Math::Vec2, ControlPoint2)
};

META_END_NAMESPACE()

#endif
