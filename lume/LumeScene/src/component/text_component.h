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

#ifndef SCENE_SRC_COMPONENT_TEXT_COMPONENT_H
#define SCENE_SRC_COMPONENT_TEXT_COMPONENT_H

#include <scene/ext/component_fwd.h>
#include <scene/interface/intf_text.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(TextComponent, "6abc426a-5fba-4920-a4da-fd4185d796de", META_NS::ObjectCategoryBits::NO_CATEGORY)

class IInternalText : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalText, "3dbbabbe-7231-4c25-9233-c66766fa5945")
public:
    META_PROPERTY(BASE_NS::string, Text)
    META_PROPERTY(BASE_NS::string, FontFamily)
    META_PROPERTY(BASE_NS::string, FontStyle)
    META_PROPERTY(float, FontSize)
    META_PROPERTY(float, Font3DThickness)
    META_PROPERTY(SCENE_NS::FontMethod, FontMethod)
};

class TextComponent : public META_NS::IntroduceInterfaces<ComponentFwd, IInternalText> {
    META_OBJECT(TextComponent, ClassId::TextComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalText, BASE_NS::string, Text, "TextComponent.text")
    SCENE_STATIC_PROPERTY_DATA(IInternalText, BASE_NS::string, FontFamily, "TextComponent.fontFamily")
    SCENE_STATIC_PROPERTY_DATA(IInternalText, BASE_NS::string, FontStyle, "TextComponent.fontStyle")
    SCENE_STATIC_PROPERTY_DATA(IInternalText, float, FontSize, "TextComponent.fontSize")
    SCENE_STATIC_PROPERTY_DATA(IInternalText, float, Font3DThickness, "TextComponent.font3DThickness")
    SCENE_STATIC_PROPERTY_DATA(IInternalText, SCENE_NS::FontMethod, FontMethod, "TextComponent.fontMethod")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Text)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, FontFamily)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, FontStyle)
    META_IMPLEMENT_PROPERTY(float, FontSize)
    META_IMPLEMENT_PROPERTY(float, Font3DThickness)
    META_IMPLEMENT_PROPERTY(SCENE_NS::FontMethod, FontMethod)

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()
#endif
