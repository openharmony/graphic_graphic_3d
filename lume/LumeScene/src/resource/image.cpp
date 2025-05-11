
#include "image.h"

#include <mutex>

#include <render/device/intf_gpu_resource_manager.h>

SCENE_BEGIN_NAMESPACE()

bool Image::SetRenderHandle(RENDER_NS::RenderHandleReference handle)
{
    if (handle.GetHandleType() != RENDER_NS::RenderHandleType::GPU_IMAGE) {
        CORE_LOG_E("Image requires GPU_IMAGE handle");
        return false;
    }
    IRenderContext::Ptr context;
    {
        std::unique_lock lock { mutex_ };
        if (handle_.GetHandle() == handle.GetHandle()) {
            return true;
        }
        handle_ = handle;
        context = context_.lock();
        if (!context) {
            return false;
        }
    }

    RENDER_NS::GpuImageDesc desc;
    context
        ->AddTask([&] {
            auto& resources = context->GetRenderer()->GetDevice().GetGpuResourceManager();
            desc = resources.GetImageDescriptor(handle);
        })
        .Wait();

    // todo: notify in right queue
    Size()->SetValue({ desc.width, desc.height });
    if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
        META_NS::Invoke<META_NS::IOnChanged>(ev);
    }
    return true;
}

SCENE_END_NAMESPACE()
