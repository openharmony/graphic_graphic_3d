
#include "render_resource.h"

#include <scene/ext/util.h>

#include <render/device/intf_gpu_resource_manager.h>

SCENE_BEGIN_NAMESPACE()

bool RenderResource::Build(const META_NS::IMetadata::Ptr& d)
{
    auto res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct RenderResource");
            return false;
        }
        context_ = context;
    }
    return res;
}

RENDER_NS::RenderHandleReference RenderResource::GetRenderHandle() const
{
    std::shared_lock lock { mutex_ };
    return handle_;
}

SCENE_END_NAMESPACE()
