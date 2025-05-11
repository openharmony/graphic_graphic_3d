
#ifndef SCENE_INTERFACE_INTF_EXTERNAL_NODE_H
#define SCENE_INTERFACE_INTF_EXTERNAL_NODE_H

#include <scene/base/types.h>

#include <core/resources/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Attachment to mark that current node is imported from external scene
 */
class IExternalNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExternalNode, "33b69e97-4ffa-465d-8126-d7c54165aef6")
public:
    virtual CORE_NS::ResourceId GetResourceId() const = 0;
    virtual void SetResourceId(CORE_NS::ResourceId id) = 0;
};

META_REGISTER_CLASS(ExternalNode, "b26e734e-c4ec-48bc-b845-d9feb6dd5de8", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IExternalNode)

#endif
