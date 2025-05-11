
#include "generic_component.h"

#include <scene/ext/util.h>

SCENE_BEGIN_NAMESPACE()

bool GenericComponent::Build(const META_NS::IMetadata::Ptr& d)
{
    if (Super::Build(d)) {
        component_ = GetBuildArg<BASE_NS::string>(d, "Component");
    }
    return !component_.empty();
}

BASE_NS::string GenericComponent::GetName() const
{
    return component_;
}

SCENE_END_NAMESPACE()