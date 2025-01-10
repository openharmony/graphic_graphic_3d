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

#ifndef API_CORE_RESOURCES_IRESOURCE_H
#define API_CORE_RESOURCES_IRESOURCE_H

#include <base/containers/string_view.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class IResource : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "597e8395-4990-492c-9cac-ad32804c0dfd" };
    using Ptr = BASE_NS::refcnt_ptr<IResource>;

    /** Get the resource's type UID.
     * @return Type UID.
     */
    virtual BASE_NS::Uid GetType() const = 0;

    /** Get the resource's URI. A resource can be queried from the resource manager with the same URI.
     * @return Resource URI.
     */
    virtual BASE_NS::string_view GetUri() const = 0;

protected:
    IResource() = default;
    virtual ~IResource() = default;

    IResource(const IResource&) = delete;
    void operator=(const IResource&) = delete;
};
CORE_END_NAMESPACE()
#endif // API_CORE_RESOURCES_IRESOURCE_H
