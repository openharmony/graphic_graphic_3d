
#ifndef SCENE_SRC_SERIALIZATION_EXTERNAL_NODE_H
#define SCENE_SRC_SERIALIZATION_EXTERNAL_NODE_H

#include <scene/interface/intf_external_node.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class ExternalNode : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IExternalNode> {
    META_OBJECT(ExternalNode, ClassId::ExternalNode, IntroduceInterfaces)
public:
    META_NS::ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        return META_NS::ObjectFlagBits::NONE;
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return id_;
    }
    void SetResourceId(CORE_NS::ResourceId id) override
    {
        id_ = id;
    }

private:
    CORE_NS::ResourceId id_;
};

SCENE_END_NAMESPACE()

#endif
