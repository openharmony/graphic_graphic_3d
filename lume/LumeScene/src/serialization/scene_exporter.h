
#ifndef SCENE_SRC_SERIALIZATION_SCENE_EXPORTER_H
#define SCENE_SRC_SERIALIZATION_SCENE_EXPORTER_H

#include <scene/interface/serialization/intf_scene_exporter.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

constexpr META_NS::Version SCENE_EXPORTER_VERSION(0, 1);
constexpr BASE_NS::string_view SCENE_EXPORTER_TYPE("Scene");

class SceneExporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneExporter> {
    META_OBJECT(SceneExporter, SCENE_NS::ClassId::SceneExporter, IntroduceInterfaces)
public:
    META_NS::ReturnError ExportScene(CORE_NS::IFile&, const IScene::ConstPtr&) override;
    META_NS::ReturnError ExportNode(CORE_NS::IFile&, const INode::ConstPtr&) override;

private:
    META_NS::IMetadata::Ptr md_;
};

SCENE_END_NAMESPACE()

#endif