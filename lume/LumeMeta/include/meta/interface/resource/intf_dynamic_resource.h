
#ifndef META_INTERFACE_RESOURCE_IDYNAMIC_RESOURCE_H
#define META_INTERFACE_RESOURCE_IDYNAMIC_RESOURCE_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The IDynamicResource interface defines an interface for changeable resources
 */
class IDynamicResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDynamicResource, "38e11d91-0c5a-41fb-bf0c-d62c6d61d9f5")
public:
    /**
     * @brief Resource changed
     */
    META_EVENT(META_NS::IOnChanged, OnResourceChanged)
};

class IReloadableResource : public IDynamicResource {
    META_INTERFACE(IDynamicResource, IReloadableResource, "924efc47-63a8-4a25-ab7d-bacacf27eee0")
public:
    /**
     * @brief Manually refresh the resource. May lead to OnResourceChanged being invoked.
     */
    virtual void Refresh() = 0;
};

META_INTERFACE_TYPE(META_NS::IDynamicResource)

META_END_NAMESPACE()

#endif
