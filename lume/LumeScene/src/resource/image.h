
#ifndef SCENE_SRC_BITMAP_H
#define SCENE_SRC_BITMAP_H

#include <scene/interface/intf_image.h>
#include <scene/interface/intf_render_target.h>
#include <scene/interface/resource/types.h>

#include <meta/interface/intf_named.h>

#include "render_resource.h"

SCENE_BEGIN_NAMESPACE()

class Image : public META_NS::IntroduceInterfaces<RenderResource, IImage, IRenderTarget, META_NS::INamed> {
    META_OBJECT(Image, ClassId::Image, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(SCENE_NS::IImage, BASE_NS::Math::UVec2, Size, BASE_NS::Math::UVec2(0, 0),
        META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)

    RENDER_NS::RenderHandleReference GetRenderHandle() const override
    {
        return RenderResource::GetRenderHandle();
    }
    bool SetRenderHandle(RENDER_NS::RenderHandleReference handle) override;

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::ImageResource.Id().ToUid();
    }
};

SCENE_END_NAMESPACE()

#endif