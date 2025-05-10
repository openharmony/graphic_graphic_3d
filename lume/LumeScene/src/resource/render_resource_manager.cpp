#include "render_resource_manager.h"

#include <scene/ext/util.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/interface/resource/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

bool RenderResourceManager::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        context_ = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context_) {
            CORE_LOG_E("Invalid arguments to construct RenderResourceManager");
            return false;
        }
    }
    return res;
}

static RENDER_NS::GpuImageDesc SetImageInfoFlags(const ImageInfo& info, RENDER_NS::GpuImageDesc& desc)
{
    desc.usageFlags |= static_cast<uint32_t>(info.usageFlags);
    desc.memoryPropertyFlags |= static_cast<uint32_t>(info.memoryFlags);
    desc.engineCreationFlags |= static_cast<uint32_t>(info.creationFlags);
    return desc;
}

static RENDER_NS::GpuImageDesc ConvertToImageDesc(const ImageCreateInfo& info)
{
    RENDER_NS::GpuImageDesc desc;

    desc.width = info.size.x;
    desc.height = info.size.y;
    desc.format = info.format;

    return SetImageInfoFlags(info.info, desc);
}

static IImage::Ptr ConstructImage(const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference handle)
{
    auto image = META_NS::GetObjectRegistry().Create<IImage>(ClassId::Image, CreateRenderContextArg(context));
    if (auto i = interface_cast<IRenderResource>(image)) {
        i->SetRenderHandle(BASE_NS::move(handle));
    }
    return image;
}

Future<IImage::Ptr> RenderResourceManager::CreateImage(const ImageCreateInfo& info, BASE_NS::vector<uint8_t> d)
{
    return context_->AddTask([=, desc = ConvertToImageDesc(info), data = BASE_NS::move(d)] {
        auto& gpuResMan = context_->GetRenderer()->GetDevice().GetGpuResourceManager();
        RENDER_NS::RenderHandleReference handle;
        if (data.empty()) {
            handle = gpuResMan.Create(desc);
        } else {
            handle = gpuResMan.Create(desc, data);
        }
        return ConstructImage(context_, BASE_NS::move(handle));
    });
}
Future<IImage::Ptr> RenderResourceManager::LoadImage(BASE_NS::string_view uri, const ImageLoadInfo& info)
{
    return context_->AddTask([=, path = BASE_NS::string(uri)] {
        auto& gpuResMan = context_->GetRenderer()->GetDevice().GetGpuResourceManager();
        auto& loader = context_->GetRenderer()->GetEngine().GetImageLoaderManager();
        auto loadResult = loader.LoadImage(path, static_cast<uint32_t>(info.loadFlags));
        if (!loadResult.success) {
            CORE_LOG_E("Failed to load image (%s): %s", path.c_str(), loadResult.error);
            return IImage::Ptr {};
        }
        auto gpuDesc = gpuResMan.CreateGpuImageDesc(loadResult.image->GetImageDesc());
        SetImageInfoFlags(info.info, gpuDesc);
        auto handle = gpuResMan.Create(uri, gpuDesc, std::move(loadResult.image));
        return ConstructImage(context_, BASE_NS::move(handle));
    });
}

static IShader::Ptr ConstructShader(const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference handle)
{
    auto shader = META_NS::GetObjectRegistry().Create<IShader>(ClassId::Shader, CreateRenderContextArg(context));
    if (auto i = interface_cast<IShaderState>(shader)) {
        auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
        i->SetShaderState(handle, man.GetGraphicsStateHandleByShaderHandle(handle));
    }
    return shader;
}

Future<IShader::Ptr> RenderResourceManager::LoadShader(BASE_NS::string_view uri)
{
    if (uri.empty()) {
        CORE_LOG_E("Cannot load shader from empty URI");
        return {};
    }
    return context_->AddTask([=, path = BASE_NS::string(uri)] {
        auto& man = context_->GetRenderer()->GetDevice().GetShaderManager();
        man.LoadShaderFile(path);
        IShader::Ptr res;
        if (auto handle = man.GetShaderHandle(path)) {
            res = ConstructShader(context_, BASE_NS::move(handle));
        }
        return res;
    });
}

SCENE_END_NAMESPACE()
