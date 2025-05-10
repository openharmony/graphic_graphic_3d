
#ifndef SCENE_INTERFACE_INODE_IMPORT_H
#define SCENE_INTERFACE_INODE_IMPORT_H

#include <scene/base/types.h>
#include <scene/interface/intf_scene.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Interface to support import another scene or nodes from another scene under a node
 */
class INodeImport : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeImport, "7fb14660-382e-4d84-a55c-b040c89a04f4")
public:
    /**
     * @brief Adds child from another scene as child of this node.
     *        The scenes must share the same context.
     *        This will make a deep copy of the node and all children/resources it references.
     * @param node Node in another scene
     * @return The child node added or null if failed
     */
    virtual Future<INode::Ptr> ImportChild(const INode::ConstPtr& node) = 0;

    /**
     * @brief Adds whole scene node hierarchy as child of this node.
     *        The scenes must share the same context.
     *        This will make a deep copy of the node and all children/resources it references.
     *        This will also copy animations
     * @param scene Scene to import
     * @param nodeName Name for the scene's root that is copied (the name is usually empty so you can give better name
     * here)
     * @return The child node added (which was the root in given scene) or null if failed
     */
    virtual Future<INode::Ptr> ImportChildScene(const IScene::ConstPtr& scene, BASE_NS::string_view nodeName) = 0;
    /**
     * @brief Adds whole scene node hierarchy as child of this node.
     * @param uri Uri of the scene resource to load.
     * @param nodeName Name for the scene's root that is copied (the name is usually empty so you can give better name
     * here)
     * @return The child node added (which was the root in given scene) or null if failed
     */
    virtual Future<INode::Ptr> ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::INodeImport)

#endif
