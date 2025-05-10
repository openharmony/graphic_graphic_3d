
#include "mesh_component.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string MeshComponent::GetName() const
{
    return "MeshComponent";
}

BASE_NS::string RenderMeshComponent::GetName() const
{
    return "RenderMeshComponent";
}

BASE_NS::string MorphComponent::GetName() const
{
    return "MorphComponent";
}

SCENE_END_NAMESPACE()
