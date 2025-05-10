
#ifndef SCENE_INTERFACE_ITEXT_H
#define SCENE_INTERFACE_ITEXT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

enum class FontMethod : uint8_t {
    /** Normal raster fonts */
    RASTER = 0,
    /** Sdf fonts */
    SDF = 1,
    /** 3D Text */
    TEXT3D = 2
};

class IText : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IText, "762c970b-456f-4405-b698-e453e60b426d")
public:
    META_PROPERTY(BASE_NS::string, Text)
    META_PROPERTY(BASE_NS::string, FontFamily)
    META_PROPERTY(BASE_NS::string, FontStyle)
    META_PROPERTY(float, FontSize)
    META_PROPERTY(float, Font3DThickness)
    META_PROPERTY(SCENE_NS::FontMethod, FontMethod)
    META_PROPERTY(BASE_NS::Math::Vec4, TextColor)
};

META_REGISTER_CLASS(TextNode, "fd2f62e8-d91f-4408-b653-038a76b08121", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::FontMethod)
META_INTERFACE_TYPE(SCENE_NS::IText)

#endif
