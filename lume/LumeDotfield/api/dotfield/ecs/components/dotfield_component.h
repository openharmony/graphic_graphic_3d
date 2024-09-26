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

#if !defined(PLUGIN_API_DOTFIELD_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define PLUGIN_API_DOTFIELD_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <cstdint>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/component_struct_macros.h>
#include <core/property/property_types.h>
#include <base/math/vector.h>

namespace Dotfield {
#endif
BEGIN_COMPONENT(IDotfieldComponentManager, DotfieldComponent)
    DEFINE_PROPERTY(BASE_NS::Math::Vec2, size, "Dimensions of the field.", 0, ARRAY_VALUE(64.f, 64.f))
    DEFINE_PROPERTY(float, pointScale, "Point scale", 0, VALUE(60.f))
    DEFINE_PROPERTY(BASE_NS::Math::Vec2, touchPosition, "Touch position", 0, ARRAY_VALUE(0.f, 0.f))
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, touchDirection, "Touch direction", 0, ARRAY_VALUE(0.f, 0.f, 0.f))
    DEFINE_PROPERTY(float, touchRadius, "Touch radius", 0, VALUE(0.f))
    DEFINE_PROPERTY(uint32_t, color0, "Color 0", 0, VALUE(0u))
    DEFINE_PROPERTY(uint32_t, color1, "Color 1", 0, VALUE(0u))
    DEFINE_PROPERTY(uint32_t, color2, "Color 2", 0, VALUE(0u))
    DEFINE_PROPERTY(uint32_t, color3, "Color 3", 0, VALUE(0u))
END_COMPONENT(IDotfieldComponentManager, DotfieldComponent, "b1711202-59ed-4735-a795-8b13e7f0af4d")
#if !defined(IMPLEMENT_MANAGER)
} // namespace Dotfield
#endif
#endif // PLUGIN_API_DOTFIELD_COMPONENT_H
