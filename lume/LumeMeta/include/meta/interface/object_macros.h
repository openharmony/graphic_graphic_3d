#ifndef META_INTERFACE_OBJECT_MACROS_H
#define META_INTERFACE_OBJECT_MACROS_H

#include <base/containers/atomics.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_lifecycle.h>

/**
 * @brief Implement reference counting with ILifecycle support for destruction.
 */
#define META_IMPLEMENT_REF_COUNT()                                         \
    int32_t refcnt_ { 0 };                                                 \
    void Ref() override                                                    \
    {                                                                      \
        BASE_NS::AtomicIncrement(&refcnt_);                                \
    }                                                                      \
    void Unref() override                                                  \
    {                                                                      \
        if (BASE_NS::AtomicDecrement(&refcnt_) == 0) {                     \
            if (auto i = this->GetInterface(::META_NS::ILifecycle::UID)) { \
                static_cast<::META_NS::ILifecycle*>(i)->Destroy();         \
            }                                                              \
            delete this;                                                   \
        }                                                                  \
    }

#endif
