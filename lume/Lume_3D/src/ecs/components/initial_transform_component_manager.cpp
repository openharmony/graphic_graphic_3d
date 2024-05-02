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

#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/initial_transform_component.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(CORE3D_NS::InitialTransformComponent::Data);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class InitialTransformComponentManager final
    : public BaseManager<InitialTransformComponent, IInitialTransformComponentManager> {
    BEGIN_PROPERTY(InitialTransformComponent, ComponentMetadata)
#include "ecs/components/initial_transform_component.h"
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit InitialTransformComponentManager(IEcs& ecs)
        : BaseManager<InitialTransformComponent, IInitialTransformComponentManager>(
              ecs, CORE_NS::GetName<InitialTransformComponent>())
    {}

    ~InitialTransformComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IInitialTransformComponentManagerInstance(IEcs& ecs)
{
    return new InitialTransformComponentManager(ecs);
}

void IInitialTransformComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<InitialTransformComponentManager*>(instance);
}

InitialTransformComponent::~InitialTransformComponent()
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }
}

InitialTransformComponent::InitialTransformComponent() : type(CORE_NS::PropertyType::FLOAT_T)
{
    initialData.floatValue = 0.f;
}

InitialTransformComponent::InitialTransformComponent(float value) : type(CORE_NS::PropertyType::FLOAT_T)
{
    initialData.floatValue = value;
}

InitialTransformComponent::InitialTransformComponent(BASE_NS::Math::Vec2 value) : type(CORE_NS::PropertyType::VEC2_T)
{
    initialData.vec2Value = value;
}

InitialTransformComponent::InitialTransformComponent(BASE_NS::Math::Vec3 value) : type(CORE_NS::PropertyType::VEC3_T)
{
    initialData.vec3Value = value;
}

InitialTransformComponent::InitialTransformComponent(BASE_NS::Math::Vec4 value) : type(CORE_NS::PropertyType::VEC4_T)
{
    initialData.vec4Value = value;
}

InitialTransformComponent::InitialTransformComponent(BASE_NS::Math::Quat value) : type(CORE_NS::PropertyType::QUAT_T)
{
    initialData.quatValue = value;
}

InitialTransformComponent::InitialTransformComponent(BASE_NS::array_view<const float> value)
    : type(CORE_NS::PropertyType::FLOAT_VECTOR_T)
{
    new (&initialData.floatVectorValue) BASE_NS::vector<float>(value.cbegin(), value.cend());
}

InitialTransformComponent::InitialTransformComponent(const InitialTransformComponent& other) noexcept : type(other.type)
{
    if (type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        BASE_NS::CloneData(&initialData, sizeof(initialData), &other.initialData, sizeof(initialData));
    } else {
        new (&initialData.floatVectorValue) BASE_NS::vector<float>(other.initialData.floatVectorValue);
    }
}

InitialTransformComponent::InitialTransformComponent(InitialTransformComponent&& other) noexcept : type(other.type)
{
    if (type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        BASE_NS::CloneData(&initialData, sizeof(initialData), &other.initialData, sizeof(initialData));
    } else {
        new (&initialData.floatVectorValue) BASE_NS::vector<float>(BASE_NS::move(other.initialData.floatVectorValue));
        other.type = CORE_NS::PropertyType::FLOAT_T;
    }
}

InitialTransformComponent& InitialTransformComponent::operator=(const InitialTransformComponent& other) noexcept
{
    if (&other != this) {
        if (other.type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
            if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
                initialData.floatVectorValue.~vector();
            }
            BASE_NS::CloneData(&initialData, sizeof(initialData), &other.initialData, sizeof(initialData));
        } else if (type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
            new (&initialData.floatVectorValue) BASE_NS::vector<float>(other.initialData.floatVectorValue);
        } else {
            initialData.floatVectorValue = other.initialData.floatVectorValue;
        }
        type = other.type;
    }
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(InitialTransformComponent&& other) noexcept
{
    if (&other != this) {
        if (other.type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
            if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
                initialData.floatVectorValue.~vector();
            }
            BASE_NS::CloneData(&initialData, sizeof(initialData), &other.initialData, sizeof(initialData));
        } else if (type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
            new (&initialData.floatVectorValue)
                BASE_NS::vector<float>(BASE_NS::move(other.initialData.floatVectorValue));
        } else {
            initialData.floatVectorValue = other.initialData.floatVectorValue;
        }
        type = BASE_NS::exchange(other.type, CORE_NS::PropertyType::FLOAT_T);
    }
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(float value) noexcept
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }

    type = CORE_NS::PropertyType::FLOAT_T;
    initialData.floatValue = value;
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(BASE_NS::Math::Vec2 value) noexcept
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }
    type = CORE_NS::PropertyType::VEC2_T;
    initialData.vec2Value = value;
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(BASE_NS::Math::Vec3 value) noexcept
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }
    type = CORE_NS::PropertyType::VEC3_T;
    initialData.vec3Value = value;
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(BASE_NS::Math::Vec4 value) noexcept
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }
    type = CORE_NS::PropertyType::VEC4_T;
    initialData.vec4Value = value;
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(BASE_NS::Math::Quat value) noexcept
{
    if (type == CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        initialData.floatVectorValue.~vector();
    }
    type = CORE_NS::PropertyType::QUAT_T;
    initialData.quatValue = value;
    return *this;
}

InitialTransformComponent& InitialTransformComponent::operator=(BASE_NS::array_view<const float> value) noexcept
{
    if (type != CORE_NS::PropertyType::FLOAT_VECTOR_T) {
        type = CORE_NS::PropertyType::FLOAT_VECTOR_T;
        new (&initialData.floatVectorValue) BASE_NS::vector<float>(value.cbegin(), value.cend());
    } else {
        initialData.floatVectorValue.clear();
        initialData.floatVectorValue.insert(initialData.floatVectorValue.cbegin(), value.cbegin(), value.cend());
    }
    return *this;
}
CORE3D_END_NAMESPACE()
