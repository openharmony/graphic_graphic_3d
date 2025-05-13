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

#if !defined(API_TEXT_3D_ECS_COMPONENTS_TEXT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_TEXT_3D_ECS_COMPONENTS_TEXT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <base/containers/string.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <text_3d/namespace.h>

TEXT3D_BEGIN_NAMESPACE()
#endif
/** Text component is used for displaying simple text lables.
 */
BEGIN_COMPONENT(ITextComponentManager, TextComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Font method type */
    enum class FontMethod : uint8_t {
        /** Normal raster fonts */
        RASTER = 0,
        /** Sdf fonts */
        SDF = 1,
        /** 3D Text */
        TEXT3D = 2
    };
#endif
    DEFINE_PROPERTY(BASE_NS::string, text, "Text", 0, )
    DEFINE_PROPERTY(BASE_NS::string, fontFamily, "Font Family", 0, )
    DEFINE_PROPERTY(BASE_NS::string, fontStyle, "Font Style", 0, )
    DEFINE_PROPERTY(float, fontSize, "Font Size", 0, )
    DEFINE_PROPERTY(float, font3DThickness, "3D Font Thickness", 0, )
    DEFINE_PROPERTY(FontMethod, fontMethod, "Font Method", 0, FontMethod::RASTER)

END_COMPONENT(ITextComponentManager, TextComponent, "66be0c20-3b2b-44c9-a54c-462235e94003")
#if !defined(IMPLEMENT_MANAGER)
TEXT3D_END_NAMESPACE()
#endif
#endif // API_TEXT_3D_ECS_COMPONENTS_TEXT_COMPONENT_H
