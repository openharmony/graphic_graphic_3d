
#include "animation_type.h"

#include <scene/ext/util.h>

SCENE_BEGIN_NAMESPACE()

bool AnimationResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct AnimationResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string AnimationResourceType::GetResourceName() const
{
    return "AnimationResource";
}
BASE_NS::Uid AnimationResourceType::GetResourceType() const
{
    return ClassId::AnimationResource.Id().ToUid();
}
CORE_NS::IResource::Ptr AnimationResourceType::LoadResource(const StorageInfo& s) const
{
    return nullptr;
}
bool AnimationResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    return true;
}
bool AnimationResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

SCENE_END_NAMESPACE()