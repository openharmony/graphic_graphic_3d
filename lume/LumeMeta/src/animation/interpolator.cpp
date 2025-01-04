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

#define META_INTERPOLATOR(ClassName, Type)                       \
    class ClassName : public Interpolator<Type> {                \
        META_OBJECT(ClassName, ClassId::ClassName, Interpolator) \
    };

META_INTERPOLATOR(FloatInterpolator, float)
META_INTERPOLATOR(DoubleInterpolator, double)
META_INTERPOLATOR(Vec2Interpolator, BASE_NS::Math::Vec2)
META_INTERPOLATOR(Vec3Interpolator, BASE_NS::Math::Vec3)
META_INTERPOLATOR(Vec4Interpolator, BASE_NS::Math::Vec4)
META_INTERPOLATOR(UVec2Interpolator, BASE_NS::Math::UVec2)
META_INTERPOLATOR(IVec2Interpolator, BASE_NS::Math::IVec2)
META_INTERPOLATOR(UInt8Interpolator, uint8_t)
META_INTERPOLATOR(UInt16Interpolator, uint16_t)
META_INTERPOLATOR(UInt32Interpolator, uint32_t)
META_INTERPOLATOR(UInt64Interpolator, uint64_t)
META_INTERPOLATOR(Int8Interpolator, int8_t)
META_INTERPOLATOR(Int16Interpolator, int16_t)
META_INTERPOLATOR(Int32Interpolator, int32_t)
META_INTERPOLATOR(Int64Interpolator, int64_t)

class QuatInterpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(QuatInterpolator, ClassId::QuatInterpolator, IntroduceInterfaces)
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

class DefaultInterpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(DefaultInterpolator, ClassId::DefaultInterpolator, IntroduceInterfaces)
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
class UVec3Interpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(UVec3Interpolator, ClassId::UVec3Interpolator, IntroduceInterfaces)
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

class UVec4Interpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(UVec4Interpolator, ClassId::UVec4Interpolator, IntroduceInterfaces)
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

class IVec3Interpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(IVec3Interpolator, ClassId::IVec3Interpolator, IntroduceInterfaces)
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
class IVec4Interpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
    META_OBJECT(IVec4Interpolator, ClassId::IVec4Interpolator, IntroduceInterfaces)
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
