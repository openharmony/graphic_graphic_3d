
#ifndef SCENE_EXT_INODE_NOTIFY_H
#define SCENE_EXT_INODE_NOTIFY_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_node.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_container.h>

SCENE_BEGIN_NAMESPACE()

class INodeNotify : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeNotify, "71ee5799-e9c7-4e3f-b724-98eb5c43bd39")
public:
    virtual bool IsListening() const = 0;
    virtual void OnChildChanged(META_NS::ContainerChangeType type, const INode::Ptr& child, size_t index) = 0;
    /// Node activity state change state
    enum class NodeActiteStateInfo : uint8_t {
        ACTIVATED,    ///< The node was activated
        DEACTIVATING, ///< The node is going to be deactivated
    };
    /// Called by the framework when node node activity state is changing
    virtual void OnNodeActiveStateChanged(NodeActiteStateInfo state) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::INodeNotify)

#endif
