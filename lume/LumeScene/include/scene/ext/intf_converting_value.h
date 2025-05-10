
#ifndef SCENE_EXT_ICONVERTING_VALUE_H
#define SCENE_EXT_ICONVERTING_VALUE_H

#include <scene/base/namespace.h>

#include <meta/interface/property/intf_property.h>

SCENE_BEGIN_NAMESPACE()

class IConvertingValue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IConvertingValue, "0ae695dd-79de-4f02-b3eb-984479f9eae1")
public:
    virtual META_NS::IProperty::Ptr GetTargetProperty() = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IConvertingValue)

#endif