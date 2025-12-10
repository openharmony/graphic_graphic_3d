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

#ifndef SCENE_SRC_COMPONENT_TRANSFORM_COMPONENT_H
#define SCENE_SRC_COMPONENT_TRANSFORM_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/interface/intf_transform.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    TransformComponent, "534bdd95-3137-4f2f-89ab-0d795560facb", META_NS::ObjectCategoryBits::NO_CATEGORY)

class TransformComponent : public META_NS::IntroduceInterfaces<Component, ITransform> {
    META_OBJECT(TransformComponent, ClassId::TransformComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Position, "TransformComponent.position")
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Scale, "TransformComponent.scale")
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Quat, Rotation, "TransformComponent.rotation")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Position)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Scale)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Quat, Rotation)

    BASE_NS::Math::Mat4X4 GetTransformMatrix() const override;

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif
