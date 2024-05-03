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
