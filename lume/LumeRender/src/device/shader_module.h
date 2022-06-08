/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    virtual ShaderSpecilizationConstantView GetSpecilization() const = 0;

    /* The following are not found from every shader, data might be zeros */

    virtual VertexInputDeclarationView GetVertexInputDeclaration() const = 0;
    virtual ShaderThreadGroup GetThreadGroupSize() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SHADER_MODULE_H
