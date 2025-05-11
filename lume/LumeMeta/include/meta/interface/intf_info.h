
#ifndef META_INTERFACE_IINFO_H
#define META_INTERFACE_IINFO_H

#include <cstdint>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Information for entity
 */
class IInfo : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInfo, "4fc7656f-f6f2-43a9-a1f6-10f03d393d1f")
public:
    /// Name of entity
    virtual BASE_NS::string_view GetName() const = 0;
    /// Description of entity
    virtual BASE_NS::string_view GetDescription() const = 0;
};

META_END_NAMESPACE()

#endif
