
#ifndef SCENE_SRC_COMPONENT_NODE_COMPONENT_H
#define SCENE_SRC_COMPONENT_NODE_COMPONENT_H

#include <scene/ext/component.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IInternalNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalNode, "7ce14037-9137-4cbd-877a-8ea8862e9fb2")
public:
    META_PROPERTY(bool, Enabled)
};

META_REGISTER_CLASS(NodeComponent, "0a222395-21cc-4518-a1ca-0cdca384ba9e", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeComponent : public META_NS::IntroduceInterfaces<Component, IInternalNode> {
    META_OBJECT(NodeComponent, ClassId::NodeComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalNode, bool, Enabled, "NodeComponent.enabled")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif