
#ifndef SCENE_INTERFACE_ISCENE_MANAGER_H
#define SCENE_INTERFACE_ISCENE_MANAGER_H

#include <scene/base/types.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/scene_options.h>

SCENE_BEGIN_NAMESPACE()

class ISceneManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneManager, "4ac48d37-5bd9-4237-87e1-09848e1e15c5")
public:
    virtual Future<IScene::Ptr> CreateScene() = 0;
    virtual Future<IScene::Ptr> CreateScene(SceneOptions) = 0;
    virtual Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri) = 0;
    virtual Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri, SceneOptions) = 0;

    virtual IRenderContext::Ptr GetContext() const = 0;
};

META_REGISTER_CLASS(SceneManager, "e294e3d0-014f-4c93-a12b-e25c691277b4", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ISceneManager)

#endif
