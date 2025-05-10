#ifndef SCENE_SRC_ASSET_ASSET_OBJECT_H
#define SCENE_SRC_ASSET_ASSET_OBJECT_H

#include <ecs_serializer/intf_ecs_asset_manager.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IAssetObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAssetObject, "70c77b11-bd85-490a-880f-d8a80d2addb0")
public:
    virtual bool Load(const IScene::Ptr&, BASE_NS::string_view uri) = 0;
};

META_REGISTER_CLASS(AssetObject, "a8b4c9e9-9c28-49b7-85b2-2eb6ac812b7c", META_NS::ObjectCategoryBits::NO_CATEGORY)

class AssetObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IAssetObject> {
    META_OBJECT(AssetObject, SCENE_NS::ClassId::AssetObject, IntroduceInterfaces)

public:
    bool Load(const IScene::Ptr&, BASE_NS::string_view uri) override;

private:
    ECS_SERIALIZER_NS::IEntityCollection::Ptr entities_;
};

SCENE_END_NAMESPACE()

#endif
