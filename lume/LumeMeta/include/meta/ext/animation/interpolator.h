
#ifndef META_EXT_INTERPOLATOR_H
#define META_EXT_INTERPOLATOR_H

#include <meta/api/util.h>
#include <meta/base/namespace.h>
#include <meta/ext/base_object.h>
#include <meta/interface/animation/intf_interpolator.h>

META_BEGIN_NAMESPACE()

/**
 * @brief A default implementation template for an interpolator for a given type.
 */
template<class Type>
class Interpolator : public IntroduceInterfaces<BaseObject, IInterpolator> {
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
