/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_PLATFORM_DATA_H
#define OHOS_RENDER_3D_PLATFORM_DATA_H

#include <string>

namespace OHOS::Render3D {
struct HapInfo {
    std::string hapPath_ = "";
    std::string bundleName_ = "";
    std::string moduleName_ = "";
};
struct PlatformData {
    /** Core root path */
    std::string coreRootPath_;
    /** Application root path */
    std::string appRootPath_;
    /** Application plugin path */
    std::string appPluginPath_;
    // hapInfo
    HapInfo hapInfo_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_PLATFORM_DATA_H
