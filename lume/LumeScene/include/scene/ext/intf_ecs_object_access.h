
#ifndef SCENE_EXT_IECS_OBJECT_ACCESS_H
#define SCENE_EXT_IECS_OBJECT_ACCESS_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_object.h>

SCENE_BEGIN_NAMESPACE()

class IEcsObjectAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsObjectAccess, "b4c430bd-29ad-41cf-96c6-16d4df74396a")
public:
    virtual bool SetEcsObject(const IEcsObject::Ptr&) = 0;
    virtual IEcsObject::Ptr GetEcsObject() const = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEcsObjectAccess)

#endif
