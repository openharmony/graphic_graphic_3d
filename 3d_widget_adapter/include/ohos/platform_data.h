/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_PLATFORM_DATA_H
#define OHOS_RENDER_3D_PLATFORM_DATA_H

#include <jni.h>
#include <string>

namespace OHOS::Render3D {
struct PlatformData {
    /** Core root path */
    std::string coreRootPath_;
    /** Application root path */
    std::string appRootPath_;
    /** Application plugin path */
    std::string appPluginPath_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_PLATFORM_DATA_H
