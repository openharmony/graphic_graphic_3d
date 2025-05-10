
#ifndef SCENE_SRC_RESOURCE_TYPES_SCENE_TYPE_H
#define SCENE_SRC_RESOURCE_TYPES_SCENE_TYPE_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/scene_options.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class SceneResourceType : public META_NS::IntroduceInterfaces<META_NS::MetaObject, CORE_NS::IResourceType> {
    META_OBJECT(SceneResourceType, ClassId::SceneResource, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    BASE_NS::string GetResourceName() const override;
    BASE_NS::Uid GetResourceType() const override;
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;

private:
    IRenderContext::WeakPtr context_;
    SceneOptions opts_;
};

SCENE_END_NAMESPACE()

#endif