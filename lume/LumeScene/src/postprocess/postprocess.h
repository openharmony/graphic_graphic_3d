
#ifndef SCENE_SRC_POSTPROCESS_POSTPROCESS_H
#define SCENE_SRC_POSTPROCESS_POSTPROCESS_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/resource/types.h>

#include <meta/api/resource/derived_from_resource.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

#include "../component/postprocess_component.h"

SCENE_BEGIN_NAMESPACE()

class PostProcess : public META_NS::IntroduceInterfaces<META_NS::DerivedFromResource, NamedSceneObject, IPostProcess,
                        ICreateEntity, META_NS::Resource> {
    META_OBJECT(PostProcess, SCENE_NS::ClassId::PostProcess, IntroduceInterfaces)
public:
    PostProcess() : Super(ClassId::PostProcessResourceTemplate) {}

    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, ITonemap::Ptr, Tonemap, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IBloom::Ptr, Bloom, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(ITonemap::Ptr, Tonemap)
    META_IMPLEMENT_READONLY_PROPERTY(IBloom::Ptr, Bloom)

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::PostProcessResource.Id().ToUid();
    }

private:
    IInternalPostProcess::Ptr pp_;
};

SCENE_END_NAMESPACE()

#endif