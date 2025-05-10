#ifndef META_EXT_EVENT_UTIL_H
#define META_EXT_EVENT_UTIL_H

#include <meta/interface/intf_metadata.h>

META_BEGIN_NAMESPACE()

template<typename MyEvent, typename MyObj, typename... Args>
inline auto InvokeByName(const MyObj& obj, BASE_NS::string_view name, Args&&... args)
{
    if (auto i = interface_cast<IMetadata>(obj)) {
        if (auto ev = i->GetEvent(name, MetadataQuery::EXISTING)) {
            if (auto event = interface_cast<typename MyEvent::InterfaceType>(ev)) {
                return event->Invoke(BASE_NS::forward<Args>(args)...);
            }
            CORE_LOG_W("Trying to Invoke wrong type of event");
        }
    } else {
        CORE_LOG_W("Trying to Invoke event by name with type that doesn't support IMetadata");
    }
    if constexpr (!BASE_NS::is_same_v<typename MyEvent::ReturnType, void>) {
        return typename MyEvent::ReturnType {};
    }
}

META_END_NAMESPACE()
#endif
