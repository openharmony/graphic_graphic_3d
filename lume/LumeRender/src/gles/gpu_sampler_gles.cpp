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

#include "gpu_sampler_gles.h"

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
GpuSamplerGLES::GpuSamplerGLES(Device& device, const GpuSamplerDesc& desc) : device_((DeviceGLES&)device), desc_(desc)
{
    PLUGIN_ASSERT(device_.IsActive());
    // NOTE: delay Create?
    constexpr GLint magFilter[] = {
        GL_NEAREST, // min is CORE_FILTER_NEAREST
        GL_LINEAR   // min is CORE_FILTER_LINEAR
    };
    constexpr GLint minFilter[] = {
        GL_NEAREST_MIPMAP_NEAREST, // min = CORE_FILTER_NEAREST mip = CORE_FILTER_NEAREST
        GL_LINEAR_MIPMAP_NEAREST,  // min = CORE_FILTER_LINEAR mip = CORE_FILTER_NEAREST
        GL_NEAREST_MIPMAP_LINEAR,  // min = CORE_FILTER_NEAREST mip = CORE_FILTER_LINEAR
        GL_LINEAR_MIPMAP_LINEAR,   // min = CORE_FILTER_LINEAR mip = CORE_FILTER_LINEAR
    };
    constexpr GLint addressMode[] = {
        GL_REPEAT,          // CORE_SAMPLER_ADDRESS_MODE_REPEAT = 0,
        GL_MIRRORED_REPEAT, // CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
        GL_CLAMP_TO_EDGE,   // CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,

        // requires gles 3.2
        // CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3, requires GL_EXT_texture_border_clamp
        // if GLES 3.1
        GL_CLAMP_TO_BORDER,

        // Not supported in gles
        GL_CLAMP_TO_EDGE, // GL_MIRROR_CLAMP_TO_EDGE,//CORE_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
    };
    constexpr GLint compareMode[] = {
        GL_NEVER,    // CORE_COMPARE_OP_NEVER is 0,
        GL_LESS,     // CORE_COMPARE_OP_LESS is 1,
        GL_EQUAL,    // CORE_COMPARE_OP_EQUAL is 2,
        GL_LEQUAL,   // CORE_COMPARE_OP_LESS_OR_EQUAL is 3,
        GL_GREATER,  // CORE_COMPARE_OP_GREATER is 4,
        GL_NOTEQUAL, // CORE_COMPARE_OP_NOT_EQUAL is 5,
        GL_GEQUAL,   // CORE_COMPARE_OP_GREATER_OR_EQUAL is 6,
        GL_ALWAYS    // CORE_COMPARE_OP_ALWAYS is 7,
        // CORE_COMPARE_OP_MAX_ENUM is 0x7FFFFFFF
    };

    glGenSamplers(1, &plat_.sampler);
    glSamplerParameteri(plat_.sampler, GL_TEXTURE_MAG_FILTER, magFilter[desc.magFilter]);

    glSamplerParameteri(plat_.sampler, GL_TEXTURE_MIN_FILTER, minFilter[desc.minFilter + desc.mipMapMode * 2]);

    glSamplerParameteri(plat_.sampler, GL_TEXTURE_WRAP_S, addressMode[desc.addressModeU]);
    glSamplerParameteri(plat_.sampler, GL_TEXTURE_WRAP_T, addressMode[desc.addressModeV]);
    glSamplerParameteri(plat_.sampler, GL_TEXTURE_WRAP_R, addressMode[desc.addressModeW]);

    // "desc.mipLodBias" can not be set here on GLES, needs a shader work around if needed!
    // "desc.enableAnisotropy" is NOT SUPPORTED! it requires EXT_texture_filter_anisotropic.
    if (desc.enableCompareOp) {
        glSamplerParameteri(plat_.sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(plat_.sampler, GL_TEXTURE_COMPARE_FUNC, compareMode[desc.compareOp]);
    } else {
        glSamplerParameteri(plat_.sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glSamplerParameteri(plat_.sampler, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    }

    glSamplerParameterf(plat_.sampler, GL_TEXTURE_MIN_LOD, desc.minLod);
    glSamplerParameterf(plat_.sampler, GL_TEXTURE_MAX_LOD, desc.maxLod);

    float color[4] = { 0.f };
    switch (desc.borderColor) {
        case CORE_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
        case CORE_BORDER_COLOR_INT_TRANSPARENT_BLACK: {
            color[0] = color[1] = color[2] = color[3] = 0.f;
            break;
        }
        case CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
        case CORE_BORDER_COLOR_INT_OPAQUE_BLACK: {
            color[0] = color[1] = color[2] = 0.f;
            color[3] = 1.f;
            break;
        }
        case CORE_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
        case CORE_BORDER_COLOR_INT_OPAQUE_WHITE: {
            color[0] = color[1] = color[2] = color[3] = 1.f;
            break;
        }
        default:
            color[0] = color[1] = color[2] = color[3] = 0.f;
            break;
    }
    glSamplerParameterfv(plat_.sampler, GL_TEXTURE_BORDER_COLOR, color);

    // "desc.enableUnnormalizedCoordinates" is NOT SUPPORTED!
}

GpuSamplerGLES::~GpuSamplerGLES()
{
    if (plat_.sampler) {
        PLUGIN_ASSERT(device_.IsActive());
        device_.DeleteSampler(plat_.sampler);
    }
}

const GpuSamplerDesc& GpuSamplerGLES::GetDesc() const
{
    return desc_;
}

const GpuSamplerPlatformDataGL& GpuSamplerGLES::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
