#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/util/color.h>

#include <meta/base/ids.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/base/type_traits.h>
#include <meta/ext/any_builder.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/loaders/intf_content_loader.h>

META_BEGIN_NAMESPACE()

// clang-format off
using BasicTypes = TypeList<
    float,
    double,
    bool,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    TypeId,
    ObjectId,
    InstanceId,
    BASE_NS::Uid,
    BASE_NS::string,
    BASE_NS::Math::Vec2,
    BASE_NS::Math::UVec2,
    BASE_NS::Math::IVec2,
    BASE_NS::Math::Vec3,
    BASE_NS::Math::UVec3,
    BASE_NS::Math::IVec3,
    BASE_NS::Math::Vec4,
    BASE_NS::Math::UVec4,
    BASE_NS::Math::IVec4,
    BASE_NS::Math::Quat,
    BASE_NS::Math::Mat3X3,
    BASE_NS::Math::Mat4X4,
    BASE_NS::Color,
    TimeSpan
    >;
using ObjectTypes = TypeList<
    SharedPtrIInterface,
    SharedPtrConstIInterface,
    WeakPtrIInterface,
    WeakPtrConstIInterface,
    IObject::Ptr,
    IObject::ConstPtr,
    IObject::WeakPtr,
    IObject::ConstWeakPtr,
    IAny::Ptr,
    IAny::ConstPtr,
    IAny::WeakPtr,
    IAny::ConstWeakPtr,
    IValue::Ptr,
    IValue::ConstPtr,
    IValue::WeakPtr,
    IValue::ConstWeakPtr,
    IContentLoader::Ptr,
    IProperty::Ptr,
    IProperty::ConstPtr,
    IProperty::WeakPtr,
    IProperty::ConstWeakPtr,
    IAnimation::Ptr,
    IAnimation::ConstPtr,
    IAnimation::WeakPtr,
    IAnimation::ConstWeakPtr,
    IAnimationController::WeakPtr,
    ICurve1D::Ptr,
    IAttach::Ptr,
    IAttach::ConstPtr,
    IAttach::WeakPtr,
    IAttach::ConstWeakPtr,
    IFunction::Ptr,
    IFunction::ConstPtr,
    IFunction::WeakPtr,
    IFunction::ConstWeakPtr
    >;
// clang-format on

namespace Internal {

template<typename... List>
static void RegisterTypes(IPropertyRegister& pr, TypeList<List...>)
{
    (pr.RegisterAny(CreateShared<DefaultAnyBuilder<Any<List>>>()), ...);
    (pr.RegisterAny(CreateShared<DefaultAnyBuilder<ArrayAny<List>>>()), ...);
}

template<typename... List>
static void UnregisterTypes(IPropertyRegister& pr, TypeList<List...>)
{
    (pr.UnregisterAny(ArrayAny<List>::StaticGetClassId()), ...);
    (pr.UnregisterAny(Any<List>::StaticGetClassId()), ...);
}

void RegisterAnys(IObjectRegistry& registry)
{
    auto& pr = registry.GetPropertyRegister();
    RegisterTypes(pr, BasicTypes {});
    RegisterTypes(pr, ObjectTypes {});

    RegisterTypeForBuiltinAny<StartableState>();
    RegisterTypeForBuiltinAny<StartBehavior>();
}

void UnRegisterAnys(IObjectRegistry& registry)
{
    auto& pr = registry.GetPropertyRegister();
    UnregisterTypes(pr, ObjectTypes {});
    UnregisterTypes(pr, BasicTypes {});

    UnregisterTypeForBuiltinAny<StartableState>();
    UnregisterTypeForBuiltinAny<StartBehavior>();
}

} // namespace Internal
META_END_NAMESPACE()
