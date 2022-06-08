/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_OS_IPLATFORM_H
#define API_CORE_OS_IPLATFORM_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <core/namespace.h>

#include "platform_create_info.h"

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

    /** Register platform specific paths
     * @param fileManager File manager instance to register paths to
     * @return Engine root directory URI
     */
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
