
#ifndef SCENE_INTERFACE_RESOURCE_IRENDER_RESOURCE_MANAGER_H
#define SCENE_INTERFACE_RESOURCE_IRENDER_RESOURCE_MANAGER_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/image_info.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Construct render resources that do _not_ depend on scene
 */
class IRenderResourceManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRenderResourceManager, "2aea6212-6557-4e6d-aba0-d6df8e437fc3")
public:
    virtual Future<IImage::Ptr> CreateImage(const ImageCreateInfo& info, BASE_NS::vector<uint8_t> data) = 0;
    Future<IImage::Ptr> CreateImage(const BASE_NS::Math::UVec2& size)
    {
        auto info = DEFAULT_IMAGE_CREATE_INFO;
        info.size = size;
        return CreateImage(info, {});
    }
    Future<IImage::Ptr> CreateImage(const ImageCreateInfo& info)
    {
        return CreateImage(info, {});
    }
    Future<IImage::Ptr> CreateRenderTarget(const BASE_NS::Math::UVec2& size)
    {
        auto info = DEFAULT_RENDER_TARGET_CREATE_INFO;
        info.size = size;
        return CreateImage(info, {});
    }
    virtual Future<IImage::Ptr> LoadImage(BASE_NS::string_view uri, const ImageLoadInfo&) = 0;
    Future<IImage::Ptr> LoadImage(BASE_NS::string_view uri)
    {
        return LoadImage(uri, DEFAULT_IMAGE_LOAD_INFO);
    }

    virtual Future<IShader::Ptr> LoadShader(BASE_NS::string_view uri) = 0;
};

META_REGISTER_CLASS(
    RenderResourceManager, "928e21c5-e086-44ba-be8d-dd7d501ff6e5", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IRenderResourceManager)

#endif
