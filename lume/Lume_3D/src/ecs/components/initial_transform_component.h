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

#if !defined(INITIAL_TRANSFORM_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define INITIAL_TRANSFORM_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(IInitialTransformComponentManager, InitialTransformComponent)
#if !defined(IMPLEMENT_MANAGER)
    union Data {
        float floatValue;

        BASE_NS::Math::Vec2 vec2Value;
        BASE_NS::Math::Vec3 vec3Value;
        BASE_NS::Math::Vec4 vec4Value;

        BASE_NS::Math::Quat quatValue;

        BASE_NS::vector<float> floatVectorValue;
#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4583) // 'floatVectorValue': destructor is not implicitly called
#endif
        ~Data() {};
#if _MSC_VER
#pragma warning(pop)
#endif
    };

    ~InitialTransformComponent();

    InitialTransformComponent();

    explicit InitialTransformComponent(float value);
    explicit InitialTransformComponent(BASE_NS::Math::Vec2 value);
    explicit InitialTransformComponent(BASE_NS::Math::Vec3 value);
    explicit InitialTransformComponent(BASE_NS::Math::Vec4 value);
    explicit InitialTransformComponent(BASE_NS::Math::Quat value);
    explicit InitialTransformComponent(BASE_NS::array_view<const float> value);

    InitialTransformComponent(const InitialTransformComponent& other) noexcept;
    InitialTransformComponent(InitialTransformComponent && other) noexcept;

    InitialTransformComponent& operator=(const InitialTransformComponent& other) noexcept;
    InitialTransformComponent& operator=(InitialTransformComponent&& other) noexcept;

    InitialTransformComponent& operator=(float value) noexcept;
    InitialTransformComponent& operator=(BASE_NS::Math::Vec2 value) noexcept;
    InitialTransformComponent& operator=(BASE_NS::Math::Vec3 value) noexcept;
    InitialTransformComponent& operator=(BASE_NS::Math::Vec4 value) noexcept;
    InitialTransformComponent& operator=(BASE_NS::Math::Quat value) noexcept;
    InitialTransformComponent& operator=(BASE_NS::array_view<const float> value) noexcept;
#endif
    DEFINE_PROPERTY(uint64_t, type, "Type Of Data",
        CORE_NS::PropertyFlags::IS_HIDDEN | CORE_NS::PropertyFlags::NO_SERIALIZE | CORE_NS::PropertyFlags::IS_READONLY,
        VALUE(0))
    DEFINE_PROPERTY(Data, initialData, "Initial Data",
        CORE_NS::PropertyFlags::IS_HIDDEN | CORE_NS::PropertyFlags::NO_SERIALIZE |
            CORE_NS::PropertyFlags::IS_READONLY, )
END_COMPONENT(IInitialTransformComponentManager, InitialTransformComponent, "3949c596-435b-4210-8a90-c8976c7168a4")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif