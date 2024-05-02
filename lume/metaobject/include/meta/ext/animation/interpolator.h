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
#ifndef META_EXT_INTERPOLATOR_H
#define META_EXT_INTERPOLATOR_H

#include <meta/api/util.h>
#include <meta/base/namespace.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_interpolator.h>

META_BEGIN_NAMESPACE()

/**
 * @brief A default implementation template for an interpolator for a given type.
 */
template<class Type, class Name, const META_NS::ClassInfo& ClassId>
class Interpolator : public META_NS::BaseObjectFwd<Name, ClassId, META_NS::ClassId::BaseObject, IInterpolator> {
public:
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsGetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            return output.SetValue<Type>(value0 + (value1 - value0) * t);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};

META_END_NAMESPACE()

#endif
