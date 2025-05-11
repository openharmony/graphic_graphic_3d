
#ifndef SCENE_INTERFACE_SERIALIZATION_ISCENE_EXPORTER_H
#define SCENE_INTERFACE_SERIALIZATION_ISCENE_EXPORTER_H

#include <scene/interface/intf_scene.h>

#include <core/io/intf_file.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class ISceneExporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneExporter, "7d8c6b65-62a2-4396-a715-801adb864bc5")
public:
    virtual META_NS::ReturnError ExportScene(CORE_NS::IFile&, const IScene::ConstPtr&) = 0;
    virtual META_NS::ReturnError ExportNode(CORE_NS::IFile&, const INode::ConstPtr&) = 0;
};

META_REGISTER_CLASS(SceneExporter, "07ec649c-4c24-45a7-bb2d-131d40e40c42", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

#endif
