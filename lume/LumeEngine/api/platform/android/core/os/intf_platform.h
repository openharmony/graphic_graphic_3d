/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
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

class IFileManager;
// Some android specific public helper functions.
class IPlatformAndroid : public IPlatform {
public:
    /** Set JavaVM for engine to use
     * NOTE: It seems that JNI_OnLoad is not called when using a native activity.
     * This can be called directly to save the vm pointer.
     * @param vm Java VM pointer
     */
    virtual void SetJavaVM(JavaVM* vm) = 0;

    /** Get JavaVM
     * @return Java VM pointer
     */
    virtual JavaVM* GetJavaVM() const = 0;

    /** Get JNIEnv
     * @return JNI Environment pointer
     */
    virtual JNIEnv* GetJavaEnv() const = 0;

    /** Get Android context
     * @return Android context object
     */
    virtual jobject GetAndroidContext() const = 0;

    /** Get application data file directory
     * @return application data file path
     */
    virtual BASE_NS::string GetAndroidFilesDir() const = 0;

    /** Get application cache file directory
     * @return application cache file path
     */
    virtual BASE_NS::string GetAndroidCacheDir() const = 0;

    /** Get application external data file directory
     * @return application external data file path
     */
    virtual BASE_NS::string GetAndroidExternalFilesDir() const = 0;

    /** Get application external cache file directory
     * @return application external cache file path
     */
    virtual BASE_NS::string GetAndroidExternalCacheDir() const = 0;

    /** Get application library directory, used for loading plugins
     * @return application library directory
     */
    virtual BASE_NS::string GetAndroidLibDir() const = 0;

    /** Get application APK paths, maybe multiple if using embedded APKs
     * @return absolute paths to application APKs, possibly inside other APKs
     */
    virtual BASE_NS::vector<BASE_NS::string> GetAndroidApkPaths() const = 0;

    /** Register APK filesystem for file manager to use
     * @param fileManager File manager instance to register to
     * @param protocol Protocol to register, usually "apk"
     */
    virtual void RegisterApkFilesystem(IFileManager& fileManager, BASE_NS::string_view protocol) const = 0;
};

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_OS_IPLATFORM_H
