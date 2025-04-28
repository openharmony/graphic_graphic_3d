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

#if RENDER_HAS_GLES_BACKEND
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#undef EGL_EGLEXT_PROTOTYPES
#endif

#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
GpuSemaphoreGles::GpuSemaphoreGles(Device& device) : device_((DeviceGLES&)device)
{
    PLUGIN_UNUSED(device_);
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
#if RENDER_HAS_GLES_BACKEND
        const auto disp = static_cast<const DevicePlatformDataGLES &>(device_.GetEglState().GetPlatformData()).display;
        EGLSyncKHR sync = reinterpret_cast<EGLSyncKHR>(plat_.sync);
        eglDestroySyncKHR(disp, sync);
#elif RENDER_HAS_GL_BACKEND
        GLsync sync = reinterpret_cast<GLsync>(plat_.sync);
        glDeleteSync(sync);
#else
        BASE_LOG_E("no available backend type");
#endif
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
