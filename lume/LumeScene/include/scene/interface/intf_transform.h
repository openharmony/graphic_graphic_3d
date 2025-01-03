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

#ifndef SCENE_INTERFACE_ITRANSFORM_H
#define SCENE_INTERFACE_ITRANSFORM_H

#include <scene/base/types.h>

#include <base/math/quaternion.h>
#include <base/math/vector.h>

SCENE_BEGIN_NAMESPACE()

class IScene;

class ITransform : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITransform, "bdf2e935-dc73-40b7-bb6e-242c131c8543")
public:
    /**
     * @brief Transform of the node Position
     */
    META_PROPERTY(BASE_NS::Math::Vec3, Position)
    /**
     * @brief Transform of the node Scale
     */
    META_PROPERTY(BASE_NS::Math::Vec3, Scale)
    /**
     * @brief Transform of the node Rotation
     */
    META_PROPERTY(BASE_NS::Math::Quat, Rotation)
    /**
     * @brief Enable node in 3D scene.
     */
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ITransform)

#endif
