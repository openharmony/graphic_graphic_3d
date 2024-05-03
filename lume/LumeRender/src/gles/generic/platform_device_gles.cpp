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

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_image_gles.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
BASE_NS::unique_ptr<GpuImage> DeviceGLES::CreateGpuImageView(
    const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData)
{
    PLUGIN_ASSERT(IsActive());
    GpuImagePlatformDataGL data {};
#if RENDER_HAS_GLES_BACKEND
    if (backendType_ == DeviceBackendType::OPENGLES) {
        const ImageDescGLES& tmp = (const ImageDescGLES&)platformData;
        data.type = tmp.type;
        data.image = tmp.image;
        data.bytesperpixel = tmp.bytesperpixel;
        data.dataType = tmp.dataType;
        data.format = tmp.format;
        data.internalFormat = tmp.internalFormat;
        data.eglImage = tmp.eglImage;
        data.hwBuffer = tmp.platformHwBuffer;
    }
#endif
#if RENDER_HAS_GL_BACKEND
    if (backendType_ == DeviceBackendType::OPENGL) {
        const ImageDescGL& tmp = (const ImageDescGL&)platformData;
        data.type = tmp.type;
        data.image = tmp.image;
        data.bytesperpixel = tmp.bytesperpixel;
        data.dataType = tmp.dataType;
        data.format = tmp.format;
        data.internalFormat = tmp.internalFormat;
    }
#endif
    data.swizzle = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
    return CreateGpuImageView(desc, data);
}
RENDER_END_NAMESPACE()
