
#ifndef META_INTERFACE_METADATA_QUERY_H
#define META_INTERFACE_METADATA_QUERY_H

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

enum class MetadataQuery {
    EXISTING,            /// Only return existing metadata objects
    CONSTRUCT_ON_REQUEST /// Create on request if not exist yet (lazy)
};

META_END_NAMESPACE()

#endif
