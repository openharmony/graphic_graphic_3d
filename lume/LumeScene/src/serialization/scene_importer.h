
#ifndef SCENE_SRC_SERIALIZATION_SCENE_IMPORTER_H
#define SCENE_SRC_SERIALIZATION_SCENE_IMPORTER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/scene_options.h>
#include <scene/interface/serialization/intf_scene_importer.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class SceneImporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneImporter> {
    META_OBJECT(SceneImporter, SCENE_NS::ClassId::SceneImporter, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr& d) override;
    IScene::Ptr ImportScene(CORE_NS::IFile&) override;
    INode::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent) override;

private:
    IRenderContext::WeakPtr context_;
    SceneOptions opts_;
};

SCENE_END_NAMESPACE()

#endif