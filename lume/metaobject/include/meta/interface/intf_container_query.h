/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef META_INTERFACE_ICONTAINER_QUERY_H
#define META_INTERFACE_ICONTAINER_QUERY_H

#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContainerQuery, "f125ea3d-7c87-4514-8fab-faf8625e7072")

/**
 * @brief The IContainerQuery interface defines an interface for querying an object
 *        for any containers it might contain that are compatible with a given set
 *        of interfaces.
 */
class IContainerQuery : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainerQuery)
public:
    /**
     * @brief The FindOptions struct defines a set of options that can be used to find containers.
     */
    struct ContainerFindOptions {
        /** The list of uids the returned containers must be compatible with.
         *  If the list is empty, all known containers are returned. */
        BASE_NS::vector<BASE_NS::Uid> uids;
        /** Maximum number of results to return. If 0, no limit will be enforced. */
        size_t maxCount {};
    };
    /**
     * @brief Returns a list of containers the object is aware of that are compatible
     *        with the set of interfaces given as a parameter.
     * @param uids The list of uids the returned containers must be compatible with.
     *             If the list is empty, all known containers are returned.
     * @param maxCount Maximum number of results to return. If 0, no limit will be enforced.
     */
    virtual BASE_NS::vector<IContainer::Ptr> FindAllContainers(const ContainerFindOptions& options) const = 0;
    /**
     * @brief Returns all containers matching the search criteria.
     */
    BASE_NS::vector<IContainer::Ptr> FindAllContainers(const BASE_NS::vector<BASE_NS::Uid>& uids) const
    {
        return FindAllContainers(ContainerFindOptions { uids, 0 });
    }
    /**
     * @brief Returns all containers which the object is aware of.
     */
    BASE_NS::vector<IContainer::Ptr> FindAllContainers() const
    {
        return FindAllContainers(ContainerFindOptions {});
    }
    /**
     * @brief Returns the matching containers for a given type.
     */
    template<class T>
    BASE_NS::vector<IContainer::Ptr> FindAllContainers() const
    {
        static_assert(IsKindOfIInterface_v<T*>, "Type must be derived from IInterface");
        return FindAllContainers({ { T::UID }, 0 });
    }
    /**
     * @brief Returns the first matching container for a given interface.
     */
    template<class T>
    IContainer::Ptr FindAnyContainer() const
    {
        static_assert(IsKindOfIInterface_v<T*>, "Type must be derived from IInterface");
        if (auto c = FindAllContainers({ { T::UID }, 1 }); !c.empty()) {
            return c.front();
        }
        return {};
    }
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ICONTAINER_QUERY_H
