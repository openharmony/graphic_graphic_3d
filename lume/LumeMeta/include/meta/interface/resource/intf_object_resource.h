
#ifndef META_INTERFACE_RESOURCE_IOBJECT_RESOURCE_H
#define META_INTERFACE_RESOURCE_IOBJECT_RESOURCE_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

class IObjectResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectResource, "273e781f-d936-4d65-a89f-72766e0fd26b")
public:
    virtual void SetResourceType(const ObjectId&) = 0;
};

META_INTERFACE_TYPE(META_NS::IObjectResource)

META_END_NAMESPACE()

#endif
