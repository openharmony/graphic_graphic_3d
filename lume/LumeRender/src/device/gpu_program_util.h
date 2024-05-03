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

#ifndef DEVICE_GPU_PROGRAM_UTIL_H
#define DEVICE_GPU_PROGRAM_UTIL_H

#include <cstdint>

#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/formats.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
namespace GpuProgramUtil {
uint32_t SpecializationByteSize(RENDER_NS::ShaderSpecialization::Constant::Type type);

void AddSpecializationConstants(
    const BASE_NS::array_view<const ShaderSpecialization::Constant> inSpecializationConstants,
    BASE_NS::vector<ShaderSpecialization::Constant>& outSpecializationConstants);
// has sorting for constants
void CombineSpecializationConstants(const BASE_NS::array_view<const ShaderSpecialization::Constant> inSc,
    BASE_NS::vector<ShaderSpecialization::Constant>& outSc);

// returns true if combination of descriptor sets is valid
[[nodiscard]] bool AddBindings(
    const DescriptorSetLayout& inDescriptorSetLayout, DescriptorSetLayout& outDescriptorSetLayout);
// has sorting for bindings and sets
void CombinePipelineLayouts(const BASE_NS::array_view<const PipelineLayout> inPl, PipelineLayout& outPl);

uint32_t FormatByteSize(BASE_NS::Format format);
} // namespace GpuProgramUtil
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_PROGRAM_UTIL_H
