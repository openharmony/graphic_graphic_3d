
#ifndef SCENE_SRC_NODE_TEXT_NODE_H
#define SCENE_SRC_NODE_TEXT_NODE_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/intf_texture.h>

#include "node.h"

SCENE_BEGIN_NAMESPACE()

class TextNode : public META_NS::IntroduceInterfaces<Node, IText, ICreateEntity> {
    META_OBJECT(TextNode, ClassId::TextNode, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, BASE_NS::string, Text)
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, BASE_NS::string, FontFamily)
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, BASE_NS::string, FontStyle)
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, float, FontSize)
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, float, Font3DThickness)
    META_STATIC_FORWARDED_PROPERTY_DATA(IText, SCENE_NS::FontMethod, FontMethod)
    META_END_STATIC_DATA()

    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::string, Text, "TextComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::string, FontFamily, "TextComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::string, FontStyle, "TextComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, FontSize, "TextComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, Font3DThickness, "TextComponent")
    SCENE_USE_COMPONENT_PROPERTY(SCENE_NS::FontMethod, FontMethod, "TextComponent")

    META_FORWARD_PROPERTY(BASE_NS::Math::Vec4, TextColor, fontColor_->Factor());

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

public:
    ITexture::Ptr fontColor_;
};

SCENE_END_NAMESPACE()

#endif
