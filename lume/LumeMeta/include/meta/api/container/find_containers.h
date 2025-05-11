
#ifndef META_API_CONTAINER_FIND_CONTAINERS_H
#define META_API_CONTAINER_FIND_CONTAINERS_H

#include <meta/api/iteration.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_required_interfaces.h>

META_BEGIN_NAMESPACE()

inline bool CheckRequiredContainerInterfaces(const IContainer::Ptr& container, const BASE_NS::vector<BASE_NS::Uid>& ids)
{
    if (ids.empty()) {
        return true;
    }
    if (auto req = interface_cast<IRequiredInterfaces>(container)) {
        const auto reqs = req->GetRequiredInterfaces();
        if (reqs.empty()) {
            return true; // Container has no requirements related to the interfaces it accepts
        }
        size_t matches = 0;
        for (const auto& req : reqs) {
            for (const auto& uid : ids) {
                if (req == uid) {
                    ++matches;
                    break;
                }
            }
        }
        return matches == ids.size();
    }

    // If container is valid but it does not implement IRequiredInterfaces, anything goes
    return container.operator bool();
}

inline BASE_NS::vector<IContainer::Ptr> FindAllContainers(
    const IObject::Ptr& obj, const IContainerQuery::ContainerFindOptions& options = {})
{
    BASE_NS::vector<IContainer::Ptr> containers;
    const auto maxCount = options.maxCount ? options.maxCount : size_t(-1);
    const auto& uids = options.uids;
    const auto addIfMatches = [&containers, &uids](const IContainer::Ptr& container) {
        if (container) {
            if (CheckRequiredContainerInterfaces(container, uids)) {
                containers.push_back(container);
            }
        }
    };
    if (const auto me = interface_pointer_cast<IContainer>(obj)) {
        // This object is itself a container
        addIfMatches(me);
    }
    if (containers.size() < maxCount) {
        if (auto att = interface_pointer_cast<IAttach>(obj)) {
            // Check the attachment container
            auto attCont = att->GetAttachmentContainer();
            addIfMatches(attCont);
            // Check the attachments themselves
            if (containers.size() < maxCount) {
                IterateShared(attCont, [&addIfMatches, &containers, &maxCount](const IObject::Ptr& object) {
                    addIfMatches(interface_pointer_cast<IContainer>(object));
                    return containers.size() < maxCount;
                });
            }
        }
    }
    return containers;
}

META_END_NAMESPACE()

#endif
