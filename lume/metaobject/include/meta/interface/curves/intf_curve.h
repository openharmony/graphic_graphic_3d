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

#ifndef META_INTERFACE_ICURVE_H
#define META_INTERFACE_ICURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICurve, "9ec41bb4-d5a9-4c18-afa4-686908e76adb")

/**
 * @brief ICurve is the base interface for all curves. A curve transforms a parameter
 *        along a curve, specified by each class which implements the ICurve interface.
 */
template<class T>
class ICurve : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICurve)
public:
    /**
     * @brief The transformation function for this curve. Transform function should be
     *        re-entrant.
     * @param t The point along the curve where the transformation should happen.
     * @return Transformed value.
     */
    virtual T Transform(float t) const = 0;
};

META_END_NAMESPACE()

#endif
