/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SHADERS__COMMON__FXAA_REFERENCE_H
#define SHADERS__COMMON__FXAA_REFERENCE_H

/** G3D version of FXAA. See copyright and warranty statement below. */

#define G3D_FXAA_PATCHES 1

// Lean towards high quality
#define FXAA_QUALITY__PRESET 29

// Explicit luma in alpha channel of input
//#define FXAA_GREEN_AS_LUMA 0

// Turn on the PC-optimized path
//#define FXAA_PC 1
#define FXAA_PC_CONSOLE 1

/** RGBA8, with values already in sRGB space and true (sRGB) luma in A.  
Must have a BILINEAR_NO_MIPMAP configuration */
//uniform_Texture(sampler2D, sourceTexture_);

/*============================================================================


NVIDIA FXAA 3.11 by TIMOTHY LOTTES
(modified for G3D with bug fixes and PC GLSL preamble)

------------------------------------------------------------------------------
COPYRIGHT (C) 2010, 2011 NVIDIA CORPORATION. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL NVIDIA
OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR
LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION,
OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE
THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.

------------------------------------------------------------------------------
INTEGRATION CHECKLIST
------------------------------------------------------------------------------
(1.)
In the shader source, setup defines for the desired configuration.
When providing multiple shaders (for different presets),
simply setup the defines differently in multiple files.
Example,

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12

Or,

#define FXAA_360 1

Or,

#define FXAA_PS3 1

Etc.

(2.)
Then include this file,

include "Fxaa3_11.h"

(3.)
Then call the FXAA pixel shader from within your desired shader.
Look at the FXAA Quality FxaaPixelShader() for docs on inputs.
As for FXAA 3.11 all inputs for all shaders are the same 
to enable easy porting between platforms.

return FxaaPixelShader(...);

(4.)
Insure pass prior to FXAA outputs RGBL (see next section).
Or use,

#define FXAA_GREEN_AS_LUMA 1

(5.)
Setup engine to provide the following constants
which are used in the FxaaPixelShader() inputs,

FxaaFloat2 fxaaQualityRcpFrame,
FxaaFloat4 fxaaConsoleRcpFrameOpt,
FxaaFloat4 fxaaConsoleRcpFrameOpt2,
FxaaFloat4 fxaaConsole360RcpFrameOpt2,
FxaaFloat fxaaQualitySubpix,
FxaaFloat fxaaQualityEdgeThreshold,
FxaaFloat fxaaQualityEdgeThresholdMin,
FxaaFloat fxaaConsoleEdgeSharpness,
FxaaFloat fxaaConsoleEdgeThreshold,
FxaaFloat fxaaConsoleEdgeThresholdMin,
FxaaFloat4 fxaaConsole360ConstDir

Look at the FXAA Quality FxaaPixelShader() for docs on inputs.

(6.)
Have FXAA vertex shader run as a full screen triangle,
and output "pos" and "fxaaConsolePosPos" 
such that inputs in the pixel shader provide,

// {xy} = center of pixel
FxaaFloat2 pos,

// {xy__} = upper left of pixel
// {__zw} = lower right of pixel
FxaaFloat4 fxaaConsolePosPos,

(7.)
Insure the texture sampler(s) used by FXAA are set to bilinear filtering.


------------------------------------------------------------------------------
INTEGRATION - RGBL AND COLORSPACE
------------------------------------------------------------------------------
FXAA3 requires RGBL as input unless the following is set, 

#define FXAA_GREEN_AS_LUMA 1

In which case the engine uses green in place of luma,
and requires RGB input is in a non-linear colorspace.

RGB should be LDR (low dynamic range).
Specifically do FXAA after tonemapping.

RGB data as returned by a texture fetch can be non-linear,
or linear when FXAA_GREEN_AS_LUMA is not set.
Note an "sRGB format" texture counts as linear,
because the result of a texture fetch is linear data.
Regular "RGBA8" textures in the sRGB colorspace are non-linear.

If FXAA_GREEN_AS_LUMA is not set,
luma must be stored in the alpha channel prior to running FXAA.
This luma should be in a perceptual space (could be gamma 2.0).
Example pass before FXAA where output is gamma 2.0 encoded,

color.rgb = ToneMap(color.rgb); // linear color output
color.rgb = sqrt(color.rgb);    // gamma 2.0 color output
return color;

To use FXAA,

color.rgb = ToneMap(color.rgb);  // linear color output
color.rgb = sqrt(color.rgb);     // gamma 2.0 color output
color.a = dot(color.rgb, FxaaFloat3(0.299, 0.587, 0.114)); // compute luma
return color;

Another example where output is linear encoded,
say for instance writing to an sRGB formated render target,
where the render target does the conversion back to sRGB after blending,

color.rgb = ToneMap(color.rgb); // linear color output
return color;

To use FXAA,

color.rgb = ToneMap(color.rgb); // linear color output
color.a = sqrt(dot(color.rgb, FxaaFloat3(0.299, 0.587, 0.114))); // compute luma
return color;

Getting luma correct is required for the algorithm to work correctly.


------------------------------------------------------------------------------
BEING LINEARLY CORRECT?
------------------------------------------------------------------------------
Applying FXAA to a framebuffer with linear RGB color will look worse.
This is very counter intuitive, but happends to be true in this case.
The reason is because dithering artifacts will be more visible 
in a linear colorspace.


------------------------------------------------------------------------------
COMPLEX INTEGRATION
------------------------------------------------------------------------------
Q. What if the engine is blending into RGB before wanting to run FXAA?

A. In the last opaque pass prior to FXAA,
have the pass write out luma into alpha.
Then blend into RGB only.
FXAA should be able to run ok
assuming the blending pass did not any add aliasing.
This should be the common case for particles and common blending passes.

A. Or use FXAA_GREEN_AS_LUMA.

============================================================================*/

/*============================================================================

INTEGRATION KNOBS

============================================================================*/
//
// FXAA_PS3 and FXAA_360 choose the console algorithm (FXAA3 CONSOLE).
// FXAA_360_OPT is a prototype for the new optimized 360 version.
//
// 1 = Use API.
// 0 = Don't use API.
//
/*--------------------------------------------------------------------------*/
#ifndef FXAA_PS3
#define FXAA_PS3 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_360
#define FXAA_360 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_360_OPT
#define FXAA_360_OPT 0
#endif
/*==========================================================================*/
#ifndef FXAA_PC
//
// FXAA Quality
// The high quality PC algorithm.
//
#define FXAA_PC 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_PC_CONSOLE
//
// The console algorithm for PC is included
// for developers targeting really low spec machines.
// Likely better to just run FXAA_PC, and use a really low preset.
//
#define FXAA_PC_CONSOLE 0
#endif

#ifndef G3D_FXAA_PATCHES
#   define G3D_FXAA_PATCHES 0
#endif

/*--------------------------------------------------------------------------*/
#ifndef FXAA_GLSL_120
#define FXAA_GLSL_120 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_GLSL_130
#define FXAA_GLSL_130 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_HLSL_3
#define FXAA_HLSL_3 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_HLSL_4
#define FXAA_HLSL_4 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_HLSL_5
#define FXAA_HLSL_5 0
#endif
/*--------------------------------------------------------------------------*/
/*==========================================================================*/
#ifndef FXAA_GREEN_AS_LUMA
//
// For those using non-linear color,
// and either not able to get luma in alpha, or not wanting to,
// this enables FXAA to run using green as a proxy for luma.
// So with this enabled, no need to pack luma in alpha.
//
// This will turn off AA on anything which lacks some amount of green.
// Pure red and blue or combination of only R and B, will get no AA.
//
// Might want to lower the settings for both,
//    fxaaConsoleEdgeThresholdMin
//    fxaaQualityEdgeThresholdMin
// In order to insure AA does not get turned off on colors 
// which contain a minor amount of green.
//
// 1 = On.
// 0 = Off.
//
#define FXAA_GREEN_AS_LUMA 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_EARLY_EXIT
//
// Controls algorithm's early exit path.
// On PS3 turning this ON adds 2 cycles to the shader.
// On 360 turning this OFF adds 10ths of a millisecond to the shader.
// Turning this off on console will result in a more blurry image.
// So this defaults to on.
//
// 1 = On.
// 0 = Off.
//
#define FXAA_EARLY_EXIT 1
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_DISCARD
//
// Only valid for PC OpenGL currently.
// Probably will not work when FXAA_GREEN_AS_LUMA = 1.
//
// 1 = Use discard on pixels which don't need AA.
//     For APIs which enable concurrent TEX+ROP from same surface.
// 0 = Return unchanged color on pixels which don't need AA.
//
#define FXAA_DISCARD 0
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_FAST_PIXEL_OFFSET
//
// Used for GLSL 120 only.
//
// 1 = GL API supports fast pixel offsets
// 0 = do not use fast pixel offsets
//
#ifdef GL_EXT_gpu_shader4
#define FXAA_FAST_PIXEL_OFFSET 1
#endif
#ifdef GL_NV_gpu_shader5
#define FXAA_FAST_PIXEL_OFFSET 1
#endif
#ifdef GL_ARB_gpu_shader5
#define FXAA_FAST_PIXEL_OFFSET 1
#endif
#ifndef FXAA_FAST_PIXEL_OFFSET
#define FXAA_FAST_PIXEL_OFFSET 0
#endif
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_GATHER4_ALPHA
//
// 1 = API supports gather4 on alpha channel.
// 0 = API does not support gather4 on alpha channel.
//
#if (FXAA_HLSL_5 == 1)
#define FXAA_GATHER4_ALPHA 1
#endif
#ifdef GL_ARB_gpu_shader5
#define FXAA_GATHER4_ALPHA 1
#endif
#ifdef GL_NV_gpu_shader5
#define FXAA_GATHER4_ALPHA 1
#endif
#ifndef FXAA_GATHER4_ALPHA
#define FXAA_GATHER4_ALPHA 0
#endif
#endif
#ifndef FXAA_360_NO_EXPBIAS
#define FXAA_360_NO_EXPBIAS 0
#endif

/*============================================================================
FXAA CONSOLE PS3 - TUNING KNOBS
============================================================================*/
#ifndef FXAA_CONSOLE__PS3_EDGE_SHARPNESS
//
// Consoles the sharpness of edges on PS3 only.
// Non-PS3 tuning is done with shader input.
//
// Due to the PS3 being ALU bound,
// there are only two safe values here: 4 and 8.
// These options use the shaders ability to a free *|/ by 2|4|8.
//
// 8.0 is sharper
// 4.0 is softer
// 2.0 is really soft (good for vector graphics inputs)
//
#if 1
#define FXAA_CONSOLE__PS3_EDGE_SHARPNESS 8.0
#endif
#if 0
#define FXAA_CONSOLE__PS3_EDGE_SHARPNESS 4.0
#endif
#if 0
#define FXAA_CONSOLE__PS3_EDGE_SHARPNESS 2.0
#endif
#endif
/*--------------------------------------------------------------------------*/
#ifndef FXAA_CONSOLE__PS3_EDGE_THRESHOLD
//
// Only effects PS3.
// Non-PS3 tuning is done with shader input.
//
// The minimum amount of local contrast required to apply algorithm.
// The console setting has a different mapping than the quality setting.
//
// This only applies when FXAA_EARLY_EXIT is 1.
//
// Due to the PS3 being ALU bound,
// there are only two safe values here: 0.25 and 0.125.
// These options use the shaders ability to a free *|/ by 2|4|8.
//
// 0.125 leaves less aliasing, but is softer
// 0.25 leaves more aliasing, and is sharper
//
#if 1
#define FXAA_CONSOLE__PS3_EDGE_THRESHOLD 0.125
#else
#define FXAA_CONSOLE__PS3_EDGE_THRESHOLD 0.25
#endif
#endif

/*============================================================================
FXAA QUALITY - TUNING KNOBS
------------------------------------------------------------------------------
NOTE the other tuning knobs are now in the shader function inputs!
============================================================================*/
#ifndef FXAA_QUALITY__PRESET
//
// Choose the quality preset.
// This needs to be compiled into the shader as it affects code.
// Best option to include multiple presets is to 
// in each shader define the preset, then include this file.
// 
// OPTIONS
// -----------------------------------------------------------------------
// 10 to 15 - default medium dither (10=fastest, 15=highest quality)
// 20 to 29 - less dither, more expensive (20=fastest, 29=highest quality)
// 39       - no dither, very expensive 
//
// NOTES
// -----------------------------------------------------------------------
// 12 = slightly faster then FXAA 3.9 and higher edge quality (default)
// 13 = about same speed as FXAA 3.9 and better than 12
// 23 = closest to FXAA 3.9 visually and performance wise
//  _ = the lowest digit is directly related to performance
// _  = the highest digit is directly related to style
// 
#define FXAA_QUALITY__PRESET 12
#endif

/*============================================================================

API PORTING

============================================================================*/
#if (FXAA_GLSL_120 == 1) || (FXAA_GLSL_130 == 1)
#define FxaaBool bool
#define FxaaDiscard discard
#define FxaaFloat float
#define FxaaFloat2 vec2
#define FxaaFloat3 vec3
#define FxaaFloat4 vec4
#define FxaaHalf float
#define FxaaHalf2 vec2
#define FxaaHalf3 vec3
#define FxaaHalf4 vec4
#define FxaaInt2 ivec2
#define FxaaSat(x) clamp(x, 0.0, 1.0)
#define FxaaTex texture2D
#define FxaaSampler sampler
#else
#define FxaaBool bool
#define FxaaDiscard clip(-1)
#define FxaaFloat float
#define FxaaFloat2 float2
#define FxaaFloat3 float3
#define FxaaFloat4 float4
#define FxaaHalf half
#define FxaaHalf2 half2
#define FxaaHalf3 half3
#define FxaaHalf4 half4
#define FxaaSat(x) saturate(x)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_GLSL_130 == 1)
// Requires "#version 130" or better
#define FxaaTexTop(t, s, p) textureLod(sampler2D(t, s), p, 0.0)
#define FxaaTexOff(t, s, p, o, r) textureLodOffset(sampler2D(t, s), p, 0.0, o)
#endif
/*--------------------------------------------------------------------------*/


/*============================================================================
GREEN AS LUMA OPTION SUPPORT FUNCTION
============================================================================*/
#if (FXAA_GREEN_AS_LUMA == 0)
FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.w; }
#else
FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.y; }
#endif    

/*============================================================================

FXAA3 CONSOLE - PC VERSION

------------------------------------------------------------------------------
Instead of using this on PC, I'd suggest just using FXAA Quality with
#define FXAA_QUALITY__PRESET 10
Or 
#define FXAA_QUALITY__PRESET 20
Either are higher quality and almost as fast as this on modern PC GPUs.
============================================================================*/
#if (FXAA_PC_CONSOLE == 1)
/*--------------------------------------------------------------------------*/
FxaaFloat4 FxaaPixelShader(
    // See FXAA Quality FxaaPixelShader() source for docs on Inputs!
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaSampler samp,
    //FxaaTex fxaaConsole360TexExpBiasNegOne,
    //FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    FxaaFloat4 fxaaConsole360ConstDir
) {
    // Corners: average luma of four corners
    FxaaFloat lumaNw = FxaaLuma(FxaaTexTop(tex, samp, fxaaConsolePosPos.xy));
    FxaaFloat lumaSw = FxaaLuma(FxaaTexTop(tex, samp, fxaaConsolePosPos.xw));
    FxaaFloat lumaNe = FxaaLuma(FxaaTexTop(tex, samp, fxaaConsolePosPos.zy));
    FxaaFloat lumaSe = FxaaLuma(FxaaTexTop(tex, samp, fxaaConsolePosPos.zw));

    // Read the middle RGB and luma
    FxaaFloat4 rgbyM = FxaaTexTop(tex, samp, pos.xy);
#   if (FXAA_GREEN_AS_LUMA == 0)
    FxaaFloat lumaM = rgbyM.w;
#   else
    FxaaFloat lumaM = rgbyM.y;
#   endif

    // Find the largest and smallest luminance value of the four corners.
    FxaaFloat lumaMaxNwSw = max(lumaNw, lumaSw);
    // Lottes' original bias to avoid later divide by zero.  We bias
    // the actual term required below.  In each case, the bias should be
    // less than one intensity gradiation at 8-bit
#   if G3D_FXAA_PATCHES == 0
    lumaNe += 1.0/384.0;
#   endif
    FxaaFloat lumaMinNwSw = min(lumaNw, lumaSw);
    FxaaFloat lumaMaxNeSe = max(lumaNe, lumaSe);
    FxaaFloat lumaMinNeSe = min(lumaNe, lumaSe);
    FxaaFloat lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);
    FxaaFloat lumaMin = min(lumaMinNeSe, lumaMinNwSw);

    /*
    // Naive: just average where there is a large luminance difference. This
    // overblurs because it doesn't consider the direction of the luminance gradient.
    if (lumaMax - lumaMin > 0.02) {
    return 
    (FxaaTexTop(tex, fxaaConsolePosPos.xy)+
    FxaaTexTop(tex, fxaaConsolePosPos.xw) +
    FxaaTexTop(tex, fxaaConsolePosPos.zy) +
    FxaaTexTop(tex, fxaaConsolePosPos.zw)) * 0.25;
    } else {
    return rgbyM;
    }
    */

    // The threshold for a luminance edge
    FxaaFloat lumaMaxScaled = lumaMax * fxaaConsoleEdgeThreshold;

    // Min, including the center
    FxaaFloat lumaMinM = min(lumaMin, lumaM);
    FxaaFloat lumaMaxScaledClamped = max(fxaaConsoleEdgeThresholdMin, lumaMaxScaled);

    // Max, including the center
    FxaaFloat lumaMaxM = max(lumaMax, lumaM);

    // Diagonal gradient (biased to avoid a later divide by zero)
    FxaaFloat dirSwMinusNe = lumaSw - lumaNe;
#   if G3D_FXAA_PATCHES
    dirSwMinusNe += 1.0 / 512.0;
#   endif        

    // Total range within a 3x3 neighborhood, for thresholding
    // whether it is worth appling AA
    FxaaFloat lumaMaxSubMinM = lumaMaxM - lumaMinM;

    // Other diagonal gradient
    FxaaFloat dirSeMinusNw = lumaSe - lumaNw;

    // If the entire range is less than the edge threshold, apply no AA
    if (lumaMaxSubMinM < lumaMaxScaledClamped) { return rgbyM; }

    // Tangent to the edge
    FxaaFloat2 dir;
    dir.x = dirSwMinusNe + dirSeMinusNw; // == (SW + SE) - (NE + NW) ~= S - N
    dir.y = dirSwMinusNe - dirSeMinusNw; // == (SW + NW) - (SE + NE) ~= W - E
    FxaaFloat2 dir1 = normalize(dir.xy);

    // Step one pixel-width along the tangent in both the positive and negative half spaces and sample there
    FxaaFloat4 rgbyN1 = FxaaTexTop(tex, samp, pos.xy - dir1 * fxaaConsoleRcpFrameOpt.zw);
    FxaaFloat4 rgbyP1 = FxaaTexTop(tex, samp, pos.xy + dir1 * fxaaConsoleRcpFrameOpt.zw);

#   if G3D_FXAA_PATCHES
    // A second sample up to two pixels away along the tangent.  This is a "min" in the original.  We increase the 
    // distance (i.e., amount of blur) as the luma contrast increases.
    FxaaFloat dirAbsMinTimesC = max(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness * 0.015;
    FxaaFloat2 dir2 = dir1.xy * min(lumaMaxSubMinM / dirAbsMinTimesC, 3);
#   else
    FxaaFloat dirAbsMinTimesC = min(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness;
    FxaaFloat2 dir2 = clamp(dir1.xy / dirAbsMinTimesC, -2, 2);
#   endif
    FxaaFloat4 rgbyN2 = FxaaTexTop(tex, samp, pos.xy - dir2 * fxaaConsoleRcpFrameOpt2.zw);
    FxaaFloat4 rgbyP2 = FxaaTexTop(tex, samp, pos.xy + dir2 * fxaaConsoleRcpFrameOpt2.zw);

    // (twice the) average of adjacent pixels on the tangent directions
    FxaaFloat4 rgbyA = rgbyN1 + rgbyP1;

    // Average of four adjacent pixels on the tangent direction
    FxaaFloat4 rgbyB = ((rgbyN2 + rgbyP2) * 0.25) + (rgbyA * 0.25);

    // If the 4-tap average along the tangent directions is much different from
    // the range of the original, go back to the 2-tap average. 
    // Otherwise, use the 4-tap average.  Considering this threshold avoids some
    // ringing and blurring at object edges
#if (FXAA_GREEN_AS_LUMA == 0)
    FxaaBool twoTap = (rgbyB.w < lumaMin) || (rgbyB.w > lumaMax);
#else
    FxaaBool twoTap = (rgbyB.y < lumaMin) || (rgbyB.y > lumaMax);
#endif
    if (twoTap) { rgbyB.xyz = rgbyA.xyz * 0.5; }

#   if G3D_FXAA_PATCHES
    // Keep some of the original contribution to avoid thin lines degrading completely and overblurring.
    // This is an addition to the original Lottes algorithm
    // rgbyB = sqrt(lerp(rgbyB * rgbyB, rgbyM * rgbyM, 0.25));  // Luminance preserving
    //rgbyB = lerp(rgbyB, rgbyM, 0.25); // Faster
    rgbyB = mix(rgbyB, rgbyM, 0.25); // Faster
#   endif
    return rgbyB; 
}
/*==========================================================================*/
#endif

/*
void main() {

    // Demonstration of required half-pixel correction. Enable this to see any input blurred, even though no FXAA is actually running
    // finalColor = texture2D(sourceTexture_buffer, (floor(g3d_TexCoord.xy * sourceTexture_size.xy) + float2(0.5, 0.5)) * sourceTexture_invSize.xy); return;

    float2 texelCenter = (floor(g3d_TexCoord.xy * sourceTexture_size.xy) + float2(0.5, 0.5)) * sourceTexture_invSize.xy;
    finalColor = FxaaPixelShader(
        // This explicit correction to half-pixel positions is needed when rendering to the hardware frame buffer
        // but not when rendering to a texture.
        texelCenter,

#       if FXAA_PC_CONSOLE
        float4(texelCenter - sourceTexture_invSize.xy * 0.5,
            texelCenter + sourceTexture_invSize.xy * 0.5),
#       else
        float4(0), // unused     FxaaFloat4 fxaaConsolePosPos,
#       endif
        sourceTexture_buffer,
        sourceTexture_buffer, // unused FxaaTex fxaaConsole360TexExpBiasNegOne,
        sourceTexture_buffer, // unused FxaaTex fxaaConsole360TexExpBiasNegTwo,
        sourceTexture_invSize.xy,
        float4(-sourceTexture_invSize.xy, sourceTexture_invSize.xy), //  FxaaFloat4 fxaaConsoleRcpFrameOpt,
        2 * float4(-sourceTexture_invSize.xy, sourceTexture_invSize.xy), //  FxaaFloat4 fxaaConsoleRcpFrameOpt2,
        float4(0), // unused FxaaFloat4 fxaaConsole360RcpFrameOpt2,
        0.75, // FxaaFloat fxaaQualitySubpix, on the range [0 no subpixel filtering but sharp to 1 maximum and blurry, .75 default]
        0.125, // FxaaFloat fxaaQualityEdgeThreshold, on the range [0.063 (slow, overkill, good) to 0.333 (fast), 0.125 default]
        0.0625, // FxaaFloat fxaaQualityEdgeThresholdMin, on the range [0.0833 (fast) to 0.0312 (good), with 0.0625 called "high quality"]
        8.0, //  FxaaFloat fxaaConsoleEdgeSharpness,
#       ifdef G3D_FXAA_PATCHES
        0.08,//0.125 default, //  FxaaFloat fxaaConsoleEdgeThreshold,
#       else
        0.125, //  FxaaFloat fxaaConsoleEdgeThreshold,
#       endif
        0.05, //  FxaaFloat fxaaConsoleEdgeThresholdMin,
        float4(0) // unused FxaaFloat4 fxaaConsole360ConstDir
    );
}
*/

#endif // SHADERS__COMMON__FXAA_REFERENCE_H
