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

#include "spirv_cross_helpers_gles.h"

#include <cmath>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace Gles {
int32_t FindConstant(
    const array_view<const PushConstantReflection> reflections, const PushConstantReflection& reflection)
{
    for (size_t i = 0; i < reflections.size(); i++) {
        if (reflection.name == reflections[i].name) {
            // Check that it's actually same and not a conflict!.
            if (reflection.type != reflections[i].type) {
                return -2;
            }
            if (reflection.offset != reflections[i].offset) {
                return -2;
            }
            if (reflection.size != reflections[i].size) {
                return -2;
            }
            if (reflection.arraySize != reflections[i].arraySize) {
                return -2;
            }
            if (reflection.arrayStride != reflections[i].arrayStride) {
                return -2;
            }
            if (reflection.matrixStride != reflections[i].matrixStride) {
                return -2;
            }
            return (int32_t)i;
        }
    }
    return -1;
}

bool DefineForSpec(const array_view<const ShaderSpecialization::Constant> reflectionInfo, const uint32_t spcid,
    const uintptr_t offset, string& result)
{
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 4294967295\n" //worst case for bool
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 4294967295\n" //worst case for uint32
    // "#define SPIRV_CROSS_CONSTANT_ID_4294967295 -2147483648\n"//worst case for int32
    // and floats can be REALLY long..
    char buf[1024];
    bool ok = false;
    for (const auto& c : reflectionInfo) {
        if (c.id == spcid) {
            // The constant_id can only be applied to a scalar *int*, a scalar *float* or a scalar *bool*.
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gl_spirv.txt
            switch (c.type) {
                default:
                    [[fallthrough]];
                case ShaderSpecialization::Constant::Type::INVALID:
                    PLUGIN_ASSERT_MSG(false, "Unhandled specialization constant type");
                    break;

                case ShaderSpecialization::Constant::Type::BOOL:
                    [[fallthrough]];
                case ShaderSpecialization::Constant::Type::UINT32: {
                    const uint32_t value = *reinterpret_cast<uint32_t*>(offset);
                    const int len = sprintf_s(buf, sizeof(buf), "%u %uu\n", c.id, value);
                    ok = len > 0;
                    break;
                }

                case ShaderSpecialization::Constant::Type::INT32: {
                    const int32_t value = *reinterpret_cast<int32_t*>(offset);
                    const int len = sprintf_s(buf, sizeof(buf), "%u %d\n", c.id, value);
                    ok = len > 0;
                    break;
                }

                case ShaderSpecialization::Constant::Type::FLOAT: {
                    const float value = *reinterpret_cast<float_t*>(offset);
                    // NOTE: resulting constant might not be the same. due to float -> string -> float conversions.
                    const int len = sprintf_s(buf, sizeof(buf), "%u %f\n", c.id, value);
                    ok = len > 0;
                    break;
                }
            }
            if (ok) {
                result.append("#define SPIRV_CROSS_CONSTANT_ID_");
                result.append(buf);
            }
            break;
        }
    }
    return ok;
}

string InsertDefines(const string_view shaderIn, const string_view Defines)
{
    string shaderOut;
    // Create defines..
    if (!shaderIn.empty()) {
        const size_t voff = shaderIn.find_first_of('\n');
        shaderOut.reserve(shaderIn.length() + Defines.length());
        shaderOut.append(shaderIn.substr(0, voff + 1));
        shaderOut.append(Defines);
        shaderOut.append(shaderIn.substr(voff + 1));
    } else {
        shaderOut = Defines;
    }
    return shaderOut;
}

string Specialize(ShaderStageFlags mask, const string_view shaderTemplate,
    const array_view<const ShaderSpecialization::Constant> info, const ShaderSpecializationConstantDataView& data)
{
    if (shaderTemplate.empty()) {
        return {};
    }
    if (data.data.empty()) {
        // missing specialization constant values
        return string(shaderTemplate);
    }
    bool ok = false;
    for (const auto& spc : data.constants) {
        if (spc.shaderStage & mask) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        // nothing to specialize
        return string(shaderTemplate);
    }
    // Create defines..
    const uintptr_t base = (uintptr_t)data.data.data();
    string defines;
    defines.reserve(256);
    for (const auto& spc : data.constants) {
        if (spc.shaderStage & mask) {
            const uintptr_t offset = base + spc.offset;
            DefineForSpec(info, spc.id, offset, defines);
        }
    }
    // inject defines to shader source.
    return InsertDefines(shaderTemplate, defines);
}
} // namespace Gles
RENDER_END_NAMESPACE()
