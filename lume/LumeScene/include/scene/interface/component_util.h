
#ifndef SCENE_INTERFACE_COMPONENT_UTIL_H
#define SCENE_INTERFACE_COMPONENT_UTIL_H

#include <scene/interface/intf_node.h>

#include <meta/interface/intf_attach.h>

SCENE_BEGIN_NAMESPACE()

template<typename Interface>
typename Interface::Ptr GetComponent(const INode::Ptr& node)
{
    auto obj = interface_cast<META_NS::IAttach>(node);
    if (obj) {
        auto att = obj->GetAttachments<Interface>();
        if (!att.empty()) {
            return att.front();
        }
    }
    return nullptr;
}

SCENE_END_NAMESPACE()

#endif