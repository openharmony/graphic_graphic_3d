/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_SHADERS_COMMON_CORE_PACKING_COMMON_H
#define API_RENDER_SHADERS_COMMON_CORE_PACKING_COMMON_H

// Compatibility macros/constants to handle top-left / bottom-left differences between Vulkan/OpenGL(ES)
#ifdef VULKAN

uvec2 PackVec4Half2x16(const vec4 up)
{
    uvec2 packed;
    packed.x = packHalf2x16(up.xy);
    packed.y = packHalf2x16(up.zw);
    return packed;
}

vec4 UnpackVec4Half2x16(const uvec2 packed)
{
    vec4 up;
    up.xy = unpackHalf2x16(packed.x);
    up.zw = unpackHalf2x16(packed.y);
    return up;
}

uvec4 Pack2Vec4Half2x16(const vec4 up0, const vec4 up1)
{
    uvec4 packed;
    packed.x = packHalf2x16(up0.xy);
    packed.y = packHalf2x16(up0.zw);
    packed.z = packHalf2x16(up1.xy);
    packed.w = packHalf2x16(up1.zw);
    return packed;
}

void UnpackVec42Half2x16(const uvec4 packed, inout vec4 up0, inout vec4 up1)
{
    up0.xy = unpackHalf2x16(packed.x);
    up0.zw = unpackHalf2x16(packed.y);
    up1.xy = unpackHalf2x16(packed.z);
    up1.zw = unpackHalf2x16(packed.w);
}

#endif

#endif // API_RENDER_SHADERS_COMMON_CORE_PACKING_COMMON_H
