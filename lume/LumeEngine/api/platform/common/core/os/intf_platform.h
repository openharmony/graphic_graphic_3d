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

#ifndef API_CORE_OS_IPLATFORM_H
#define API_CORE_OS_IPLATFORM_H

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct PlatformData;
class IPluginRegister;
class IFileManager;

class IPlatform {
public:
    using Ptr = BASE_NS::unique_ptr<IPlatform>;

    /** Get platform specific data
     * @return Platform specific data struct
     */
    virtual const PlatformData& GetPlatformData() const = 0;
    virtual BASE_NS::string RegisterDefaultPaths(IFileManager& fileManager) = 0;
    /** Register platform specific plugin locations
     * @param registry Plugin registry instance to register paths to
     */
    virtual void RegisterPluginLocations(IPluginRegister& registry) = 0;

protected:
    // Needed for unique_ptr usage
    friend BASE_NS::default_delete<IPlatform>;
    IPlatform() = default;
    virtual ~IPlatform() = default;
};

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_OS_IPLATFORM_H
