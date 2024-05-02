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
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>

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
    IContentLoader::Ptr,
    IProperty::WeakPtr,
    IAnimation::Ptr,
    IAnimation::WeakPtr,
    IAnimationController::WeakPtr,
    ICurve1D::Ptr,
    IAttach::WeakPtr,
    IFunction::Ptr,
    IFunction::ConstPtr,
    IFunction::WeakPtr
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
}

void UnRegisterAnys(IObjectRegistry& registry)
{
    auto& pr = registry.GetPropertyRegister();
    UnregisterTypes(pr, ObjectTypes {});
    UnregisterTypes(pr, BasicTypes {});
}

} // namespace Internal
META_END_NAMESPACE()
