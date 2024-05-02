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

#include "interpolator.h"

#include <base/math/quaternion_util.h>

META_BEGIN_NAMESPACE()

class FloatInterpolator : public Interpolator<float, FloatInterpolator, ClassId::FloatInterpolator> {};
class DoubleInterpolator : public Interpolator<double, DoubleInterpolator, ClassId::DoubleInterpolator> {};
class Vec2Interpolator : public Interpolator<BASE_NS::Math::Vec2, Vec2Interpolator, ClassId::Vec2Interpolator> {};
class Vec3Interpolator : public Interpolator<BASE_NS::Math::Vec3, Vec3Interpolator, ClassId::Vec3Interpolator> {};
class Vec4Interpolator : public Interpolator<BASE_NS::Math::Vec4, Vec4Interpolator, ClassId::Vec4Interpolator> {};
class UVec2Interpolator : public Interpolator<BASE_NS::Math::UVec2, UVec2Interpolator, ClassId::UVec2Interpolator> {};
class IVec2Interpolator : public Interpolator<BASE_NS::Math::IVec2, IVec2Interpolator, ClassId::IVec2Interpolator> {};
class UInt8Interpolator : public Interpolator<uint8_t, UInt8Interpolator, ClassId::UInt8Interpolator> {};
class UInt16Interpolator : public Interpolator<uint16_t, UInt16Interpolator, ClassId::UInt16Interpolator> {};
class UInt32Interpolator : public Interpolator<uint32_t, UInt32Interpolator, ClassId::UInt32Interpolator> {};
class UInt64Interpolator : public Interpolator<uint64_t, UInt64Interpolator, ClassId::UInt64Interpolator> {};
class Int8Interpolator : public Interpolator<int8_t, Int8Interpolator, ClassId::Int8Interpolator> {};
class Int16Interpolator : public Interpolator<int16_t, Int16Interpolator, ClassId::Int16Interpolator> {};
class Int32Interpolator : public Interpolator<int32_t, Int32Interpolator, ClassId::Int32Interpolator> {};
class Int64Interpolator : public Interpolator<int64_t, Int64Interpolator, ClassId::Int64Interpolator> {};

class QuatInterpolator : public META_NS::BaseObjectFwd<QuatInterpolator, ClassId::QuatInterpolator,
                             META_NS::ClassId::BaseObject, IInterpolator> {
    using Type = BASE_NS::Math::Quat;
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            return output.SetValue<Type>(BASE_NS::Math::Slerp(value0, value1, t));
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }

    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};

class DefaultInterpolator : public META_NS::BaseObjectFwd<DefaultInterpolator, ClassId::DefaultInterpolator,
                                META_NS::ClassId::BaseObject, IInterpolator> {
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        // The default interpolator doesn't know how to interpolate, so we just set the property value to either
        // from or to based on t
        if (t > .5f) {
            return output.CopyFrom(to);
        }
        return output.CopyFrom(from);
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        // Default interpolator is compatible with any type
        return true;
    }
};

// No math operations have been defined for integer type vec3/4, implement interpolators manually
class UVec3Interpolator : public META_NS::BaseObjectFwd<UVec3Interpolator, ClassId::UVec3Interpolator,
                              META_NS::ClassId::BaseObject, IInterpolator> {
    using Type = BASE_NS::Math::UVec3;
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            Type value;
            value.x = value0.x + (value1.x - value0.x) * t;
            value.y = value0.y + (value1.y - value0.y) * t;
            value.z = value0.z + (value1.z - value0.z) * t;
            return output.SetValue<Type>(value);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};

class UVec4Interpolator : public META_NS::BaseObjectFwd<UVec4Interpolator, ClassId::UVec4Interpolator,
                              META_NS::ClassId::BaseObject, IInterpolator> {
    using Type = BASE_NS::Math::UVec4;
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            Type value;
            value.x = value0.x + (value1.x - value0.x) * t;
            value.y = value0.y + (value1.y - value0.y) * t;
            value.z = value0.z + (value1.z - value0.z) * t;
            value.w = value0.w + (value1.w - value0.w) * t;
            return output.SetValue<Type>(value);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};

class IVec3Interpolator : public META_NS::BaseObjectFwd<IVec3Interpolator, ClassId::IVec3Interpolator,
                              META_NS::ClassId::BaseObject, IInterpolator> {
    using Type = BASE_NS::Math::IVec3;
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            Type value;
            value.x = value0.x + (value1.x - value0.x) * t;
            value.y = value0.y + (value1.y - value0.y) * t;
            value.z = value0.z + (value1.z - value0.z) * t;
            return output.SetValue<Type>(value);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};
class IVec4Interpolator : public META_NS::BaseObjectFwd<IVec4Interpolator, ClassId::IVec4Interpolator,
                              META_NS::ClassId::BaseObject, IInterpolator> {
    using Type = BASE_NS::Math::IVec4;
    AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const override
    {
        if (IsSetCompatibleWith<Type>(output) && IsGetCompatibleWith<Type>(from) && IsGetCompatibleWith<Type>(to)) {
            Type value0 = GetValue<Type>(from);
            Type value1 = GetValue<Type>(to);
            Type value;
            value.x = value0.x + (value1.x - value0.x) * t;
            value.y = value0.y + (value1.y - value0.y) * t;
            value.z = value0.z + (value1.z - value0.z) * t;
            value.w = value0.w + (value1.w - value0.w) * t;
            return output.SetValue<Type>(value);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    bool IsCompatibleWith(TypeId id) const noexcept override
    {
        return id == UidFromType<Type>();
    }
};

namespace BuiltInInterpolators {

struct InterpolatorInfo {
    ObjectTypeInfo OBJECT_INFO;
    TypeId propertyTypeUid;
    ObjectId interpolatorClassUid;
};

static constexpr InterpolatorInfo BUILT_IN_INTERPOLATOR_INFO[] = {
    { FloatInterpolator::OBJECT_INFO, UidFromType<float>(), ClassId::FloatInterpolator },
    { DoubleInterpolator::OBJECT_INFO, UidFromType<double>(), ClassId::DoubleInterpolator },
    { Vec2Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::Vec2>(), ClassId::Vec2Interpolator },
    { Vec3Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::Vec3>(), ClassId::Vec3Interpolator },
    { Vec4Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::Vec4>(), ClassId::Vec4Interpolator },
    { UVec2Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::UVec2>(), ClassId::UVec2Interpolator },
    { UVec3Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::UVec3>(), ClassId::UVec3Interpolator },
    { UVec4Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::UVec4>(), ClassId::UVec4Interpolator },
    { IVec2Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::IVec2>(), ClassId::IVec2Interpolator },
    { IVec3Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::IVec3>(), ClassId::IVec3Interpolator },
    { IVec4Interpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::IVec4>(), ClassId::IVec4Interpolator },
    { UInt8Interpolator::OBJECT_INFO, UidFromType<uint8_t>(), ClassId::UInt8Interpolator },
    { UInt16Interpolator::OBJECT_INFO, UidFromType<uint16_t>(), ClassId::UInt16Interpolator },
    { UInt32Interpolator::OBJECT_INFO, UidFromType<uint32_t>(), ClassId::UInt32Interpolator },
    { UInt64Interpolator::OBJECT_INFO, UidFromType<uint64_t>(), ClassId::UInt64Interpolator },
    { Int8Interpolator::OBJECT_INFO, UidFromType<int8_t>(), ClassId::Int8Interpolator },
    { Int16Interpolator::OBJECT_INFO, UidFromType<int16_t>(), ClassId::Int16Interpolator },
    { Int32Interpolator::OBJECT_INFO, UidFromType<int32_t>(), ClassId::Int32Interpolator },
    { Int64Interpolator::OBJECT_INFO, UidFromType<int64_t>(), ClassId::Int64Interpolator },
    { QuatInterpolator::OBJECT_INFO, UidFromType<BASE_NS::Math::Quat>(), ClassId::QuatInterpolator },
};

} // namespace BuiltInInterpolators

void RegisterDefaultInterpolators(IObjectRegistry& registry)
{
    for (const auto& info : BuiltInInterpolators::BUILT_IN_INTERPOLATOR_INFO) {
        // Register the classes themselves
        registry.RegisterObjectType(info.OBJECT_INFO.GetFactory());
        // Then register them as interpolators (note that default interpolator doesn't need to be registered)
        registry.RegisterInterpolator(info.propertyTypeUid, info.interpolatorClassUid.ToUid());
    }

    // No need to register the default interpolator, only register the class
    registry.RegisterObjectType<DefaultInterpolator>();
}
void UnRegisterDefaultInterpolators(IObjectRegistry& registry)
{
    for (const auto& info : BuiltInInterpolators::BUILT_IN_INTERPOLATOR_INFO) {
        // Unregister interpolator
        registry.UnregisterInterpolator(info.propertyTypeUid);
        // Unregister object
        registry.UnregisterObjectType(info.OBJECT_INFO.GetFactory());
    }
    // Unregister default interpolator
    registry.UnregisterObjectType<DefaultInterpolator>();
}

META_END_NAMESPACE()
