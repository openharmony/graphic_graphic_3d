
#ifndef META_INTERFACE_RESOURCE_IRESOURCE_CONSUMER_H
#define META_INTERFACE_RESOURCE_IRESOURCE_CONSUMER_H

#include <core/resources/intf_resource_manager.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface to access resource manager for objects that use it
 */
class IResourceConsumer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResourceConsumer, "88d10710-8a6c-466b-829c-f9ba3b3f6a14")
public:
    virtual CORE_NS::IResourceManager::Ptr GetResourceManager() const = 0;
};

META_INTERFACE_TYPE(META_NS::IResourceConsumer)

META_END_NAMESPACE()

#endif
