
#ifndef SCENE_EXT_IENGINE_PROPERTY_INIT_H
#define SCENE_EXT_IENGINE_PROPERTY_INIT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

class IEnginePropertyInit : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEnginePropertyInit, "98e0d977-229c-4c0a-bf10-978f042f557c")
public:
    virtual bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) = 0;
    virtual bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEnginePropertyInit)

#endif