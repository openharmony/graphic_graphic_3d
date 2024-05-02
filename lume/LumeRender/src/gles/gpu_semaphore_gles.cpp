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

#include "gpu_semaphore_gles.h"

#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
GpuSemaphoreGles::GpuSemaphoreGles(Device& device) : device_((DeviceGLES&)device)
{
    PLUGIN_ASSERT(device_.IsActive());
}

GpuSemaphoreGles::GpuSemaphoreGles(Device& device, const uint64_t handle)
    : device_((DeviceGLES&)device), ownsResources_(false)
{
    PLUGIN_ASSERT(device_.IsActive());
    // do not let invalid inputs in
    PLUGIN_ASSERT(handle != 0);
    if (handle) {
        plat_.sync = handle;
    }
}

GpuSemaphoreGles::~GpuSemaphoreGles()
{
    if (ownsResources_ && plat_.sync) {
        PLUGIN_ASSERT(device_.IsActive());
        GLsync sync = reinterpret_cast<GLsync>(plat_.sync);
        glDeleteSync(sync);
    }
    plat_.sync = 0;
}

uint64_t GpuSemaphoreGles::GetHandle() const
{
    return plat_.sync;
}

const GpuSemaphorePlatformDataGles& GpuSemaphoreGles::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
