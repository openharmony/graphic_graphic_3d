
#ifndef SCENE_EXT_ICREATE_ENTITY_H
#define SCENE_EXT_ICREATE_ENTITY_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_context.h>

SCENE_BEGIN_NAMESPACE()

class ICreateEntity : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICreateEntity, "707f099e-ba2a-46f4-b0c4-573480b8899a")
public:
    virtual CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ICreateEntity)

#endif
