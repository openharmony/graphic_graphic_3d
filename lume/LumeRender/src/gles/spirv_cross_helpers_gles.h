/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
