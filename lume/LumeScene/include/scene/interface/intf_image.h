
#ifndef SCENE_INTERFACE_IIMAGE_H
#define SCENE_INTERFACE_IIMAGE_H

#include <scene/base/namespace.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IImage : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImage, "a9459966-38c4-46bb-bec5-3b5d96843b5e")
public:
    META_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)
};

META_REGISTER_CLASS(Image, "77b38a92-8182-4562-bd3f-deab7b40cedc", META_NS::ObjectCategoryBits::NO_CATEGORY)

// compatibility with old
using IBitmap = IImage;
namespace ClassId {
[[maybe_unused]] inline constexpr META_NS::ClassInfo Bitmap = Image;
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IImage)

#endif
