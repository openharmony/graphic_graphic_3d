/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DEVICE_SHADER_MODULE_H
#define DEVICE_SHADER_MODULE_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct ShaderModulePlatformData {};

/** Combined shader module.
 * Compiled single shader module and reflection.
 */
class ShaderModule {
public:
    ShaderModule() = default;
    virtual ~ShaderModule() = default;

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    virtual ShaderStageFlags GetShaderStageFlags() const = 0;

    virtual const ShaderModulePlatformData& GetPlatformData() const = 0;

    virtual const PipelineLayout& GetPipelineLayout() const = 0;
    virtual ShaderSpecializationConstantView GetSpecilization() const = 0;

    /* The following are not found from every shader, data might be zeros */

    virtual VertexInputDeclarationView GetVertexInputDeclaration() const = 0;
    virtual ShaderThreadGroup GetThreadGroupSize() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SHADER_MODULE_H
