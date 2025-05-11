
#ifndef SCENE_API_EXTERNAL_NODE_H
#define SCENE_API_EXTERNAL_NODE_H

#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <core/resources/intf_resource.h>

#include <meta/interface/intf_attach.h>

SCENE_BEGIN_NAMESPACE()

inline INode::Ptr AddExternalNode(INode& parent, const IScene::Ptr& external, BASE_NS::string_view nodeName)
{
    INode::Ptr node;
    if (auto i = interface_cast<INodeImport>(&parent)) {
        node = i->ImportChildScene(external, nodeName).GetResult();
        if (node) {
            if (auto ext = META_NS::GetObjectRegistry().Create<IExternalNode>(ClassId::ExternalNode)) {
                if (auto att = interface_cast<META_NS::IAttach>(node)) {
                    if (auto res = interface_cast<CORE_NS::IResource>(external)) {
                        ext->SetResourceId(res->GetResourceId());
                    }
                    att->Attach(ext);
                }
            }
        }
    }
    return node;
}

SCENE_END_NAMESPACE()

#endif // SCENE_API_EXTERNAL_NODE_H
