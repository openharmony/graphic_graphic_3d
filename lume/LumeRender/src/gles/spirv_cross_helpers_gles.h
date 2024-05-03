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
#ifndef GLES_SPIRV_CROSS_HELPERS_H
#define GLES_SPIRV_CROSS_HELPERS_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>

#include "spirv_cross_helper_structs_gles.h"

RENDER_BEGIN_NAMESPACE()
namespace Gles {

bool DefineForSpec(BASE_NS::array_view<const ShaderSpecialization::Constant> reflectionInfo, uint32_t spcid,
    uintptr_t offset, BASE_NS::string& result);

BASE_NS::string InsertDefines(BASE_NS::string_view shaderIn, BASE_NS::string_view Defines);

BASE_NS::string Specialize(ShaderStageFlags mask, BASE_NS::string_view shaderTemplate,
    BASE_NS::array_view<const ShaderSpecialization::Constant> info, const ShaderSpecializationConstantDataView& data);

int32_t FindConstant(
    BASE_NS::array_view<const PushConstantReflection> reflections, const PushConstantReflection& reflection);

} // namespace Gles
RENDER_END_NAMESPACE()
#endif // GLES_SPIRV_CROSS_HELPERS_H
