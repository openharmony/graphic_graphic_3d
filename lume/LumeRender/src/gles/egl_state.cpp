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

#if RENDER_HAS_GLES_BACKEND

#include "egl_state.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_VERSION_1_5
// If egl 1.5 headers not available, just define the values here.
// (copied from khronos specifications)
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_OPENGL_ES3_BIT 0x00000040
#define EGL_CONTEXT_OPENGL_DEBUG 0x31B0
#define EGL_GL_COLORSPACE 0x309D
#define EGL_GL_COLORSPACE_SRGB 0x3089
#define EGL_GL_COLORSPACE_LINEAR 0x308A
typedef intptr_t EGLAttrib;
#endif

#ifndef EGL_KHR_create_context
// If EGL_KHR_create_context extension not defined in headers, so just define the values here.
// (copied from khronos specifications)
#define EGL_CONTEXT_MAJOR_VERSION_KHR 0x3098
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

#ifndef EGL_KHR_gl_colorspace
// If EGL_KHR_gl_colorspace extension not defined in headers, so just define the values here.
// (copied from khronos specifications)
#define EGL_GL_COLORSPACE_KHR 0x309D
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#define EGL_GL_COLORSPACE_LINEAR_KHR 0x308A
#endif /* EGL_KHR_gl_colorspace */

#ifndef EGL_KHR_no_config_context
// If #ifndef EGL_KHR_no_config_context extension not defined in headers, so just define the values here.
// (copied from khronos specifications)
#define EGL_NO_CONFIG_KHR ((EGLConfig)0)
#endif

#include <securec.h>

#include <render/namespace.h>

#include "gles/gl_functions.h"
#include "util/log.h"

#define declare(a, b) \
    extern "C" {      \
    a b = nullptr;    \
    }
// NOTE: intentional re-include of gl_functions.h
#undef GLES_FUNCTIONS_H
#include "gles/gl_functions.h"
#define declare(a, b) \
    extern "C" {      \
    a b = nullptr;    \
    }
// NOTE: intentional re-include of gl_functions.h
#undef EGL_FUNCTIONS_H
#include "gles/egl_functions.h"
#include "gles/surface_information.h"
#include "gles/swapchain_gles.h"

#if RENDER_GL_DEBUG
#define CHECK_EGL_ERROR() EGLHelpers::CheckEGLError(PLUGIN_FILE_INFO)
#define CHECK_EGL_ERROR2() EGLHelpers::CheckEGLError2(PLUGIN_FILE_INFO)
#else
#define CHECK_EGL_ERROR()
#define CHECK_EGL_ERROR2() EGLHelpers::CheckEGLError2(PLUGIN_FILE_INFO)
#endif

RENDER_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

namespace EGLHelpers {
namespace {
bool FilterError(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const string_view, const void*) noexcept
{
    if (source == GL_DEBUG_SOURCE_OTHER) {
        if (type == GL_DEBUG_TYPE_PERFORMANCE) {
            if ((id == 2147483647) && (severity == GL_DEBUG_SEVERITY_HIGH)) { // 2147483647: message id
                /*  Ignore the following warning that Adreno drivers seem to spam.
                source: GL_DEBUG_SOURCE_OTHER
                type: GL_DEBUG_TYPE_PERFORMANCE
                id: 2147483647
                severity: GL_DEBUG_SEVERITY_HIGH
                message: FreeAllocationOnTimestamp - WaitForTimestamp
                */
                return true;
            }
        }
    }
    return false;
}

const char* EglErrorStr(EGLint aError)
{
    switch (aError) {
        case EGL_SUCCESS:
            return "The last function succeeded without error.";
        case EGL_NOT_INITIALIZED:
            return "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
        case EGL_BAD_ACCESS:
            return "EGL cannot access a requested resource(for example a context is bound in another thread).";
        case EGL_BAD_ALLOC:
            return "EGL failed to allocate resources for the requested operation.";
        case EGL_BAD_ATTRIBUTE:
            return "An unrecognized attribute or attribute value was passed in the attribute list.";
        case EGL_BAD_CONTEXT:
            return "An EGLContext argument does not name a valid EGL rendering context.";
        case EGL_BAD_CONFIG:
            return "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
        case EGL_BAD_CURRENT_SURFACE:
            return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer "
                   "valid.";
        case EGL_BAD_DISPLAY:
            return "An EGLDisplay argument does not name a valid EGL display connection.";
        case EGL_BAD_SURFACE:
            return "An EGLSurface argument does not name a valid surface(window, pixel buffer or pixmap) configured "
                   "for GL rendering.";
        case EGL_BAD_MATCH:
            return "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid "
                   "surface).";
        case EGL_BAD_PARAMETER:
            return "One or more argument values are invalid.";
        case EGL_BAD_NATIVE_PIXMAP:
            return "A NativePixmapType argument does not refer to a valid native pixmap.";
        case EGL_BAD_NATIVE_WINDOW:
            return "A NativeWindowType argument does not refer to a valid native window.";
        case EGL_CONTEXT_LOST:
            return "A power management event has occurred.The application must destroy all contexts and reinitialise "
                   "OpenGL ES state and objects to continue rendering.";
        default:
            break;
    }

    static char error[64];
    if (sprintf_s(error, sizeof(error), "Unknown error %x", aError) < 0) {
        PLUGIN_LOG_E("EglErrorStr: sprintf_s failed");
    }
    return error;
}

void CheckEGLError(const char* const file, int line)
{
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        PLUGIN_LOG_E("eglGetError failed : %s (%s %d)", EglErrorStr(error), file, line);
        PLUGIN_ASSERT(false);
    }
}

void CheckEGLError2(const char* const file, int line)
{
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        PLUGIN_LOG_E("eglGetError failed : %s (%s %d)", EglErrorStr(error), file, line);
    }
}

// clang-format off
#define ATTRIBUTE(_attr) { _attr, #_attr }
// clang-format on

struct Attribute {
    EGLint attribute;
    const char* const name;
};

void DumpEGLStrings(EGLDisplay dpy)
{
    // extensions dumped later.
    static constexpr Attribute strings[] = { ATTRIBUTE(EGL_CLIENT_APIS), ATTRIBUTE(EGL_VENDOR),
        ATTRIBUTE(EGL_VERSION) };
    for (auto string : strings) {
        const char* const value = eglQueryString(dpy, string.attribute);
        if (value) {
            PLUGIN_LOG_D("\t%-32s: %s", string.name, value);
        } else {
            PLUGIN_LOG_D("\t%-32s:", string.name);
        }
    }
}

#ifdef PLUGIN_UNUSED_EGL_HELPERS
void DumpEGLConfigs(EGLDisplay dpy)
{
    EGLConfig* configs = nullptr;
    EGLint n = 0;

    eglGetConfigs(dpy, NULL, 0, &n);
    configs = new EGLConfig[(size_t)n];
    eglGetConfigs(dpy, configs, n, &n);
    for (EGLint i = 0; i < n; i++) {
        PLUGIN_LOG_V("EGLConfig[%d]", i);
        DumpEGLConfig(dpy, configs[i]);
    }
    delete[] configs;
}
#endif

bool StringToUInt(string_view string, EGLint& value)
{
    value = 0;
    for (auto digit : string) {
        value *= 10; // 10: decimal base
        if ((digit >= '0') && (digit <= '9')) {
            value += digit - '0';
        } else {
            return false;
        }
    }
    return true;
}

void ParseMajorAndMinor(const string_view version, EGLint& major, EGLint& minor)
{
    const auto dot = version.find_first_of('.');
    if (dot != string_view::npos) {
        const auto majorS = version.substr(0, dot);
        if (!StringToUInt(majorS, major)) {
            major = 0;
        }
        const auto space = version.find_first_of(' ', dot + 1);
        if (space != string_view::npos) {
            auto minorS = version.substr(dot + 1, space - (dot + 1));
            if (!StringToUInt(minorS, minor)) {
                minor = 0;
            }
        }
    }
}

bool ParseGlVersion(string_view versionString, EGLint& glMajor, EGLint& glMinor)
{
    // the format according to spec pdf is "OpenGL ES N.M vendor-specific information"
    static constexpr string_view glesString = "OpenGL ES ";
    if (!versionString.starts_with(glesString)) {
        return false;
    }
    // Must be OpenGL ES FULL. Trust this information. (even if it might mismatch with the eglQueryContext
    // results)
    versionString.remove_prefix(glesString.size());
    ParseMajorAndMinor(versionString, glMajor, glMinor);
    return glMajor < 1;
}

void DumpEGLSurface(EGLDisplay dpy, EGLSurface surf)
{
    static constexpr Attribute attribs[] = {
        // Returns the ID of the EGL frame buffer configuration with respect to which the surface was created.
        ATTRIBUTE(EGL_CONFIG_ID),
        // Returns the color space used by OpenGL and OpenGL ES when rendering to the surface, either
        // EGL_GL_COLORSPACE_SRGB or EGL_GL_COLORSPACE_LINEAR.
        ATTRIBUTE(EGL_GL_COLORSPACE),
        // Returns the height of the surface in pixels
        ATTRIBUTE(EGL_HEIGHT),
        // Returns the horizontal dot pitch of the display on which a window surface is  visible.The value returned is
        // equal to the actual dot pitch, in pixels meter, multiplied by the constant value EGL_DISPLAY_SCALING.
        ATTRIBUTE(EGL_HORIZONTAL_RESOLUTION),
        // Returns the same attribute value specified when the surface was created with  eglCreatePbufferSurface.For a
        // window or pixmap surface, value is not modified.
        ATTRIBUTE(EGL_LARGEST_PBUFFER),
        ATTRIBUTE(EGL_MIPMAP_LEVEL),   // Returns which level of the mipmap to render to, if texture has mipmaps.
        ATTRIBUTE(EGL_MIPMAP_TEXTURE), // Returns EGL_TRUE if texture has mipmaps, EGL_FALSE otherwise.
        // Returns the filter used when resolving the multisample buffer.The filter may be either
        // EGL_MULTISAMPLE_RESOLVE_DEFAULT or EGL_MULTISAMPLE_RESOLVE_BOX, as described for eglSurfaceAttrib.
        ATTRIBUTE(EGL_MULTISAMPLE_RESOLVE),
        // Returns the aspect ratio of an individual pixel(the ratio of a pixel's width to its height). The value
        // returned is equal to the actual aspect ratio multiplied by the constant value EGL_DISPLAY_SCALING.
        ATTRIBUTE(EGL_PIXEL_ASPECT_RATIO),
        // Returns the buffer which client API rendering is requested to use.For a window surface, this is the same
        // attribute value specified when the surface was created.For a pbuffer surface, it is always
        // EGL_BACK_BUFFER.For a pixmap surface, it is always EGL_SINGLE_BUFFER.To determine the actual buffer being
        // rendered to by a context, call eglQueryContext.
        ATTRIBUTE(EGL_RENDER_BUFFER),
        // Returns the effect on the color buffer when posting a surface with eglSwapBuffers.Swap behavior may be either
        // EGL_BUFFER_PRESERVED or EGL_BUFFER_DESTROYED, as described for eglSurfaceAttrib.
        ATTRIBUTE(EGL_SWAP_BEHAVIOR),
        // Returns format of texture.Possible values are EGL_NO_TEXTURE, EGL_TEXTURE_RGB, and EGL_TEXTURE_RGBA.
        ATTRIBUTE(EGL_TEXTURE_FORMAT),
        ATTRIBUTE(EGL_TEXTURE_TARGET), // Returns type of texture.Possible values are EGL_NO_TEXTURE, or EGL_TEXTURE_2D.
        // Returns the vertical dot pitch of the display on which a window surface is visible.The value returned is
        // equal to the actual dot pitch, in pixels  / meter, multiplied by the constant value EGL_DISPLAY_SCALING.
        ATTRIBUTE(EGL_VERTICAL_RESOLUTION),
        // Returns the interpretation of alpha values used by OpenVG when rendering to the surface, either
        // EGL_VG_ALPHA_FORMAT_NONPRE or EGL_VG_ALPHA_FORMAT_PRE.
        ATTRIBUTE(EGL_VG_ALPHA_FORMAT),
        // Returns the color space used by OpenVG when rendering to the surface, either EGL_VG_COLORSPACE_sRGB or
        // EGL_VG_COLORSPACE_LINEAR.
        ATTRIBUTE(EGL_VG_COLORSPACE),
        ATTRIBUTE(EGL_WIDTH), // Returns the width of the surface in pixels.
    };
    for (auto attrib : attribs) {
        EGLint value;
        if (EGL_TRUE == eglQuerySurface(dpy, surf, attrib.attribute, &value)) {
            PLUGIN_LOG_V("\t%-32s: %10d (0x%08x)", attrib.name, value, value);
        }
    }
}

void DumpEGLConfig(EGLDisplay dpy, const EGLConfig& config)
{
    static constexpr Attribute attributes[] = {
        ATTRIBUTE(EGL_ALPHA_SIZE),      // Returns the number of bits of alpha stored in the color buffer.
        ATTRIBUTE(EGL_ALPHA_MASK_SIZE), // Returns the number of bits in the alpha mask buffer.
        // Returns EGL_TRUE if color buffers can be bound to an RGB texture,  EGL_FALSE otherwise.
        ATTRIBUTE(EGL_BIND_TO_TEXTURE_RGB),
        // Returns EGL_TRUE if color buffers can be bound to an RGBA texture, EGL_FALSE otherwise.
        ATTRIBUTE(EGL_BIND_TO_TEXTURE_RGBA),
        ATTRIBUTE(EGL_BLUE_SIZE), // Returns the number of bits of blue stored in the color buffer.
        // Returns the depth of the color buffer.It is the sum of EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, and
        // EGL_ALPHA_SIZE.
        ATTRIBUTE(EGL_BUFFER_SIZE),
        // Returns the color buffer type.Possible types are EGL_RGB_BUFFER and  EGL_LUMINANCE_BUFFER.
        ATTRIBUTE(EGL_COLOR_BUFFER_TYPE),
        // Returns the caveats for the frame buffer configuration.Possible caveat values  are EGL_NONE, EGL_SLOW_CONFIG,
        // and EGL_NON_CONFORMANT.
        ATTRIBUTE(EGL_CONFIG_CAVEAT),
        ATTRIBUTE(EGL_CONFIG_ID), // Returns the ID of the frame buffer configuration.
        // Returns a bitmask indicating which client API contexts created with respect to this
        // config are conformant.
        ATTRIBUTE(EGL_CONFORMANT),
        ATTRIBUTE(EGL_DEPTH_SIZE), // Returns the number of bits in the depth buffer.
        ATTRIBUTE(EGL_GREEN_SIZE), // Returns the number of bits of green stored in the color buffer.
        // Returns the frame buffer level.Level zero is the default frame buffer.Positive levels correspond to frame
        // buffers that overlay the default buffer and negative levels correspond to frame buffers that underlay the
        // default buffer.
        ATTRIBUTE(EGL_LEVEL),
        ATTRIBUTE(EGL_LUMINANCE_SIZE),     // Returns the number of bits of luminance stored in the luminance buffer.
        ATTRIBUTE(EGL_MAX_PBUFFER_WIDTH),  // Returns the maximum width of a pixel buffer surface in pixels.
        ATTRIBUTE(EGL_MAX_PBUFFER_HEIGHT), // Returns the maximum height of a pixel buffer surface in pixels.
        ATTRIBUTE(EGL_MAX_PBUFFER_PIXELS), // Returns the maximum size of a pixel buffer surface in pixels.
        ATTRIBUTE(EGL_MAX_SWAP_INTERVAL),  // Returns the maximum value that can be passed to eglSwapInterval.
        ATTRIBUTE(EGL_MIN_SWAP_INTERVAL),  // Returns the minimum value that can be passed to eglSwapInterval.
        // Returns EGL_TRUE if native rendering APIs can render into the surface, EGL_FALSE otherwise.
        ATTRIBUTE(EGL_NATIVE_RENDERABLE),
        ATTRIBUTE(EGL_NATIVE_VISUAL_ID),   // Returns the ID of the associated native visual.
        ATTRIBUTE(EGL_NATIVE_VISUAL_TYPE), // Returns the type of the associated native visual.
        ATTRIBUTE(EGL_RED_SIZE),           // Returns the number of bits of red stored in the color buffer.
        ATTRIBUTE(EGL_RENDERABLE_TYPE),    // Returns a bitmask indicating the types of supported client API contexts.
        ATTRIBUTE(EGL_SAMPLE_BUFFERS),     // Returns the number of multisample buffers.
        ATTRIBUTE(EGL_SAMPLES),            // Returns the number of samples per pixel.
        ATTRIBUTE(EGL_STENCIL_SIZE),       // Returns the number of bits in the stencil buffer.
        ATTRIBUTE(EGL_SURFACE_TYPE),       // Returns a bitmask indicating the types of supported EGL surfaces.
        // Returns the type of supported transparency.Possible transparency values are  EGL_NONE, and
        // EGL_TRANSPARENT_RGB.
        ATTRIBUTE(EGL_TRANSPARENT_TYPE),
        ATTRIBUTE(EGL_TRANSPARENT_RED_VALUE),   // Returns the transparent red value.
        ATTRIBUTE(EGL_TRANSPARENT_GREEN_VALUE), // Returns the transparent green value.
        ATTRIBUTE(EGL_TRANSPARENT_BLUE_VALUE),  // Returns the transparent blue value.
        // ATTRIBUTE(EGL_MATCH_NATIVE_PIXMAP),  While EGL_MATCH_NATIVE_PIXMAP can be specified in the attribute list
        // passed to eglChooseConfig, it is not an attribute of the resulting config and cannot be queried using
        // eglGetConfigAttrib.
    };
    for (auto attribute : attributes) {
        EGLint value;
        if (EGL_TRUE == eglGetConfigAttrib(dpy, config, attribute.attribute, &value)) {
            PLUGIN_LOG_V("\t%-32s: %10d (0x%08x)", attribute.name, value, value);
        }
    }
}

void ParseExtensions(const string& extensions, vector<string_view>& extensionList)
{
    size_t start = 0;
    for (auto end = extensions.find(' '); end != string::npos; end = extensions.find(' ', start)) {
        extensionList.emplace_back(extensions.data() + start, end - start);
        start = end + 1;
    }
    if (start < extensions.size()) {
        extensionList.emplace_back(extensions.data() + start);
    }
}

void FillProperties(DevicePropertiesGLES& properties)
{
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &properties.maxCombinedTextureImageUnits);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &properties.maxCubeMapTextureSize);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &properties.maxFragmentUniformVectors);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &properties.maxRenderbufferSize);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &properties.maxTextureImageUnits);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &properties.maxTextureSize);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &properties.maxVaryingVectors);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &properties.maxVertexAttribs);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &properties.maxVertexTextureImageUnits);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &properties.maxVertexUniformVectors);
    glGetFloatv(GL_MAX_VIEWPORT_DIMS, properties.maxViewportDims);
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &properties.numCompressedTextureFormats);
    glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &properties.numShaderBinaryFormats);
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &properties.numProgramBinaryFormats);

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &properties.max3DTextureSize);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &properties.maxArrayTextureLayers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &properties.maxColorAttachments);
    glGetInteger64v(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, &properties.maxCombinedFragmentUniformComponents);
    glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &properties.maxCombinedUniformBlocks);
    glGetInteger64v(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, &properties.maxCombinedVertexUniformComponents);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &properties.maxDrawBuffers);
    glGetInteger64v(GL_MAX_ELEMENT_INDEX, &properties.maxElementIndex);
    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &properties.maxElementsIndices);
    glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &properties.maxElementsVertices);
    glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &properties.maxFragmentInputComponents);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &properties.maxFragmentUniformBlocks);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &properties.maxFragmentUniformComponents);
    glGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &properties.minProgramTexelOffset);
    glGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &properties.maxProgramTexelOffset);
    glGetIntegerv(GL_MAX_SAMPLES, &properties.maxSamples);
    glGetInteger64v(GL_MAX_SERVER_WAIT_TIMEOUT, &properties.maxServerWaitTimeout);
    glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &properties.maxTextureLodBias);
    glGetIntegerv(
        GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &properties.maxTransformFeedbackInterleavedComponents);
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &properties.maxTransformFeedbackSeparateAttribs);
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &properties.maxTransformFeedbackSeparateComponents);
    glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &properties.maxUniformBlockSize);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &properties.maxUniformBufferBindings);
    glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &properties.maxVaryingComponents);
    glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &properties.maxVertexOutputComponents);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &properties.maxVertexUniformBlocks);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &properties.maxVertexUniformComponents);

    glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &properties.maxAtomicCounterBufferBindings);
    glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, &properties.maxAtomicCounterBufferSize);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &properties.maxColorTextureSamples);
    glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTERS, &properties.maxCombinedAtomicCounters);
    glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, &properties.maxCombinedAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, &properties.maxCombinedComputeUniformComponents);
    glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, &properties.maxCombinedImageUniforms);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &properties.maxCombinedShaderOutputResources);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &properties.maxCombinedShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &properties.maxComputeAtomicCounters);
    glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, &properties.maxComputeAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &properties.maxComputeImageUniforms);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &properties.maxComputeShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &properties.maxComputeSharedMemorySize);
    glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &properties.maxComputeTextureImageUnits);
    glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &properties.maxComputeUniformBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &properties.maxComputeUniformComponents);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, properties.maxComputeWorkGroupCount);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, properties.maxComputeWorkGroupCount + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, properties.maxComputeWorkGroupCount + 2); // 2: index
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &properties.maxComputeWorkGroupInvocations);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, properties.maxComputeWorkGroupSize);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, properties.maxComputeWorkGroupSize + 1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, properties.maxComputeWorkGroupSize + 2); // 2: index
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &properties.maxDepthTextureSamples);
    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &properties.maxFragmentAtomicCounters);
    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &properties.maxFragmentAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &properties.maxFragmentImageUniforms);
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &properties.maxFragmentShaderStorageBlocks);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &properties.maxFramebufferHeight);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &properties.maxFramebufferSamples);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &properties.maxFramebufferWidth);
    glGetIntegerv(GL_MAX_IMAGE_UNITS, &properties.maxImageUnits);
    glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &properties.maxIntegerSamples);
    glGetIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.minProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &properties.maxProgramTextureGatherOffset);
    glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &properties.maxSampleMaskWords);
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &properties.maxShaderStorageBlockSize);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &properties.maxShaderStorageBufferBindings);
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &properties.maxUniformLocations);
    glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &properties.maxVertexAtomicCounters);
    glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &properties.maxVertexAtomicCounterBuffers);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &properties.maxVertexAttribBindings);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &properties.maxVertexAttribRelativeOffset);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &properties.maxVertexAttribStride);
    glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &properties.maxVertexImageUniforms);
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &properties.maxVertexShaderStorageBlocks);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &properties.uniformBufferOffsetAlignment);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &properties.shaderStorageBufferOffsetAlignment);

    glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE, &properties.minSampleShadingValue);
    glGetIntegerv(GL_MAX_DEBUG_GROUP_STACK_DEPTH, &properties.maxDebugGroupStackDepth);
    glGetIntegerv(GL_MAX_DEBUG_LOGGED_MESSAGES, &properties.maxDebugLoggedMessages);
    glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &properties.maxDebugMessageLength);
    glGetFloatv(GL_MIN_FRAGMENT_INTERPOLATION_OFFSET, &properties.minFragmentInterpolationOffset);
    glGetFloatv(GL_MAX_FRAGMENT_INTERPOLATION_OFFSET, &properties.maxFragmentInterpolationOffset);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &properties.maxFramebufferLayers);
    glGetIntegerv(GL_MAX_LABEL_LENGTH, &properties.maxLabelLength);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &properties.maxTextureBufferSize);
}

bool IsSurfaceColorspaceSupported(const DevicePlatformDataGLES& plat)
{
    // Check if EGL supports sRGB color space (either EGL is > 1.5 or EGL_KHR_gl_colorspace extension is supported).
    if (plat.majorVersion > 1u || (plat.majorVersion == 1u && plat.minorVersion >= 5u)) {
        // EGL 1.5 or newer -> no need to check the extension.
        return true;
    }
    // Check if the sRGB color space extension is supported.
    return plat.hasColorSpaceExt;
}
} // namespace

#undef ATTRIBUTE
void EGLState::HandleExtensions()
{
    if (plat_.minorVersion > 4) { // 4: egl version
        cextensions_ = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        CHECK_EGL_ERROR();
        PLUGIN_LOG_D("\t%-32s: %s", "EGL_EXTENSIONS (CLIENT)", cextensions_.c_str());
        ParseExtensions(cextensions_, cextensionList_);
    }

    dextensions_ = eglQueryString(plat_.display, EGL_EXTENSIONS);
    CHECK_EGL_ERROR();
    ParseExtensions(dextensions_, dextensionList_);
    PLUGIN_LOG_D("\t%-32s: %s", "EGL_EXTENSIONS (DISPLAY)", dextensions_.c_str());
}

void EGLState::FillSurfaceInfo(const EGLDisplay display, const EGLSurface surface, const EGLint configId,
    const EGLConfig config, GlesImplementation::SurfaceInfo& res) const
{
    res.configId = static_cast<uint32_t>(configId);

    // Fetch surface parameters
    EGLint value = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &value);
    res.width = static_cast<uint32_t>(value);
    eglQuerySurface(display, surface, EGL_HEIGHT, &value);
    res.height = static_cast<uint32_t>(value);

    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &value);
    res.red_size = static_cast<uint32_t>(value);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &value);
    res.green_size = static_cast<uint32_t>(value);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &value);
    res.blue_size = static_cast<uint32_t>(value);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &value);
    res.alpha_size = static_cast<uint32_t>(value);

    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &value);
    res.depth_size = static_cast<uint32_t>(value);

    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &value);
    res.stencil_size = static_cast<uint32_t>(value);

    eglGetConfigAttrib(display, config, EGL_SAMPLES, &value);
    res.samples = static_cast<uint32_t>(value);

    EGLint colorspace = 0;
    if (IsVersionGreaterOrEqual(1, 5) || hasColorSpaceExt_) { // 1, 5: egl version
        static_assert(EGL_GL_COLORSPACE == EGL_GL_COLORSPACE_KHR);
        static_assert(EGL_GL_COLORSPACE_SRGB == EGL_GL_COLORSPACE_SRGB_KHR);
        if (eglQuerySurface(display, surface, EGL_GL_COLORSPACE, &colorspace)) {
            // EGL_GL_COLORSPACE_SRGB or EGL_GL_COLORSPACE_LINEAR.
            res.srgb = (colorspace == EGL_GL_COLORSPACE_SRGB);
        } else if (eglQuerySurface(display, surface, EGL_VG_COLORSPACE, &colorspace)) {
            // EGL_VG_COLORSPACE_sRGB or EGL_VG_COLORSPACE_LINEAR.
            res.srgb = (colorspace == EGL_VG_COLORSPACE_sRGB);
        }
    }

    if (colorspace == 0) {
        // surface is linear (no conversion made during read/write)
        // data should be srgb though.
        PLUGIN_LOG_E("EGL_GL_COLORSPACE query failed (or not available). Defaulting to linear buffer with srgb data");
        res.srgb = false;
    }
}

uint32_t EGLState::MajorVersion() const
{
    return plat_.majorVersion;
}

uint32_t EGLState::MinorVersion() const
{
    return plat_.minorVersion;
}

BASE_NS::vector<EGLint> EGLState::BuildConfigAttributes(const BackendExtraGLES* backendConfig)
{
    // construct attribute list dynamically
    vector<EGLint> attributes;
    const size_t ATTRIBUTE_RESERVE = 16;       // reserve 16 attributes
    attributes.reserve(ATTRIBUTE_RESERVE * 2); // 2 EGLints per attribute
    auto addAttribute = [&attributes](EGLint a, EGLint b) {
        attributes.push_back(a);
        attributes.push_back(b);
    };
    // Request OpenGL ES 3.x configs
    if (IsVersionGreaterOrEqual(1, 5)) { //  1, 5: egl version
        // EGL 1.5+
        addAttribute(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT);
        addAttribute(EGL_CONFORMANT, EGL_OPENGL_ES3_BIT);
    } else if (HasExtension("EGL_KHR_create_context")) {
        // "EGL_KHR_create_context"
        addAttribute(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR);
        addAttribute(EGL_CONFORMANT, EGL_OPENGL_ES3_BIT_KHR);
    } else {
        // We might be in trouble now.
        addAttribute(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT);
        addAttribute(EGL_CONFORMANT, EGL_OPENGL_ES2_BIT);
    }
    addAttribute(EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT);
    addAttribute(EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
    addAttribute(EGL_RED_SIZE, 8);   // 8: bits per component
    addAttribute(EGL_GREEN_SIZE, 8); // 8: bits per component
    addAttribute(EGL_BLUE_SIZE, 8);  // 8: bits per component
    addAttribute(EGL_CONFIG_CAVEAT, EGL_NONE);
    if (backendConfig) {
        if (backendConfig->MSAASamples > 1) {
            addAttribute(EGL_SAMPLES, (EGLint)backendConfig->MSAASamples);
            addAttribute(EGL_SAMPLE_BUFFERS, 1);
        }
        addAttribute(EGL_ALPHA_SIZE, (EGLint)backendConfig->alphaBits);
        addAttribute(EGL_DEPTH_SIZE, (EGLint)backendConfig->depthBits);
        addAttribute(EGL_STENCIL_SIZE, (EGLint)backendConfig->stencilBits);
        PLUGIN_LOG_I("Samples:%d Alpha:%d Depth:%d Stencil:%d", backendConfig->MSAASamples, backendConfig->alphaBits,
            backendConfig->depthBits, backendConfig->stencilBits);
    } else {
        addAttribute(EGL_ALPHA_SIZE, 8); // 8: bits per component
    }
    addAttribute(EGL_NONE, EGL_NONE); // terminate list
    return attributes;
}

void EGLState::ChooseConfiguration(const BackendExtraGLES* backendConfig)
{
    EGLint num_configs;
    vector<EGLint> attributes = BuildConfigAttributes(backendConfig);
    eglChooseConfig(plat_.display, attributes.data(), &plat_.config, 1, &num_configs);
    CHECK_EGL_ERROR2();

    // Check if config exists, otherwise try headless config without EGL_WINDOW_BIT.
    if (num_configs == 0) {
        PLUGIN_LOG_E("eglChooseConfig: No configuration found.");
        // 6th integer has the attribute for EGL_SURFACE_TYPE.
        attributes[5u] = EGL_PBUFFER_BIT;
        eglChooseConfig(plat_.display, attributes.data(), &plat_.config, 1, &num_configs);
        CHECK_EGL_ERROR();

        if (num_configs > 0) {
            PLUGIN_LOG_W("eglChooseConfig: Using headless config.");
        }
    }

#if RENDER_GL_DEBUG
    PLUGIN_LOG_I("eglChooseConfig returned:");
    DumpEGLConfig(plat_.display, plat_.config);
#endif
}

void EGLState::CreateContext(const BackendExtraGLES* backendConfig)
{
    vector<EGLint> context_attributes;
    const size_t ATTRIBUTE_RESERVE = 16;               // reserve 16 attributes
    context_attributes.reserve(ATTRIBUTE_RESERVE * 2); // 2 EGLints per attribute
    auto addAttribute = [&context_attributes](EGLint a, EGLint b) {
        context_attributes.push_back(a);
        context_attributes.push_back(b);
    };
    if (IsVersionGreaterOrEqual(1, 5)) { // 1, 5: egl version
        // egl 1.5 or greater.
        addAttribute(EGL_CONTEXT_MAJOR_VERSION, 3); // Select an OpenGL ES 3.x context
        addAttribute(EGL_CONTEXT_MINOR_VERSION, 2); // Select an OpenGL ES x.2 context
#if RENDER_GL_DEBUG
        // should use EGL_CONTEXT_OPENGL_DEBUG , but at least PowerVR simulator fails with this
        if (HasExtension("EGL_KHR_create_context")) {
            // Setting up debug context with the extension seems to work.
            addAttribute(EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
        }
#endif
    } else if (HasExtension("EGL_KHR_create_context")) {
        // egl 1.4 with EGL_KHR_create_context
        addAttribute(EGL_CONTEXT_MAJOR_VERSION_KHR, 3); // Select an OpenGL ES 3.x context
        addAttribute(EGL_CONTEXT_MINOR_VERSION_KHR, 2); // Select an OpenGL ES x.2 context
#if RENDER_GL_DEBUG
        addAttribute(EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR);
#endif
    } else {
        // fallback to non-extended or pre 1.5 EGL, we expect 3.2 OpenGL context, this is now checked later.
        addAttribute(EGL_CONTEXT_CLIENT_VERSION, 3); // Select an OpenGL ES 3.x context
    }
    addAttribute(EGL_NONE, EGL_NONE);

    EGLContext sharedContext = EGL_NO_CONTEXT;
    if (backendConfig) {
        sharedContext = backendConfig->sharedContext;
    }
    const EGLConfig cfg = hasConfiglessExt_ ? EGL_NO_CONFIG_KHR : plat_.config;
    PLUGIN_LOG_I("Creating new context in DeviceGLES, using shared context: %d", static_cast<bool>(sharedContext));
    plat_.context = eglCreateContext(plat_.display, cfg, sharedContext, context_attributes.data());
    CHECK_EGL_ERROR();

    if (plat_.context == EGL_NO_CONTEXT) {
        PLUGIN_LOG_E("eglCreateContext failed : %s", EglErrorStr(eglGetError()));
        PLUGIN_ASSERT(false);
        return;
    }
}

bool EGLState::IsVersionGreaterOrEqual(uint32_t major, uint32_t minor) const
{
    if (plat_.majorVersion < major) {
        return false;
    }
    if (plat_.majorVersion > major) {
        return true;
    }
    if (plat_.minorVersion >= minor) {
        return true;
    }
    return false;
}

bool EGLState::VerifyVersion()
{
    // Verify that we have at least 3.2 context.
    SaveContext();
    SetContext(nullptr); // activate the context with the dummy PBuffer.
    EGLint glMajor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
    EGLint glMinor = 0;
    glGetIntegerv(GL_MINOR_VERSION, &glMinor);
    string_view string((const char*)glGetString(GL_VERSION));
    bool fail = (glGetError() != GL_NO_ERROR);
    if (fail) {
        fail = ParseGlVersion(string, glMajor, glMinor);
    }
    if (fail) {
        // Try these then, if parsing the GL_VERSION string failed
        glMajor = glMinor = 0;
        eglQueryContext(plat_.display, plat_.context, EGL_CONTEXT_MAJOR_VERSION, &glMajor);
        eglQueryContext(plat_.display, plat_.context, EGL_CONTEXT_MINOR_VERSION_KHR, &glMinor);
    }

    if (glMajor < 3) { // 3: gl version
        fail = true;
    } else if (glMajor == 3) { // 3: gl version
        if (glMinor < 2) {     // 2: gl version
            // We do NOT support 3.0 or 3.1
            fail = true;
        }
    }

    if (fail) {
        // restore contexts and cleanup.
        PLUGIN_LOG_E(
            "Could not to Initialize required OpenGL ES version [%d.%d] [%s]", glMajor, glMinor, string.data());
        RestoreContext();
        // destroy the dummy surface also.
        eglDestroySurface(plat_.display, dummySurface_);
        dummyContext_.context = EGL_NO_CONTEXT;
        dummyContext_.drawSurface = dummyContext_.readSurface = EGL_NO_SURFACE;
        dummyContext_.display = EGL_NO_DISPLAY;
    }
    return !fail;
}

enum class DesiredOutput { SDR_LINEAR, SDR_SRGB, HDR_1010102, HDR_SCRGB_F16 };

struct BackbufferReq {
    bool needDefaultDepthStencil = false;
    int minDepthBits = 0;
    int minStencilBits = 0;
    int msaaSamples = 0;
};

static EGLConfig TryChoose(EGLDisplay dpy, const EGLint* attrs)
{
    EGLConfig cfg = EGL_NO_CONFIG_KHR;
    EGLint n = 0;
    if (eglChooseConfig(dpy, attrs, &cfg, 1, &n) && n > 0)
        return cfg;
    return EGL_NO_CONFIG_KHR;
}

static void Add(BASE_NS::vector<EGLint>& a, EGLint k, EGLint v)
{
    a.push_back(k);
    a.push_back(v);
}

static EGLConfig TryPattern(EGLDisplay dpy, const BackbufferReq& req, int r, int g, int b, int aBits, int depthBits,
    int stencilBits, bool wantFloatComponents, int msaaSamples)
{
    BASE_NS::vector<EGLint> at;
    Add(at, EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT);
    Add(at, EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
    Add(at, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT); // or ES2 fallback if needed

    Add(at, EGL_RED_SIZE, r);
    Add(at, EGL_GREEN_SIZE, g);
    Add(at, EGL_BLUE_SIZE, b);
    Add(at, EGL_ALPHA_SIZE, aBits);

    if (req.needDefaultDepthStencil) {
        Add(at, EGL_DEPTH_SIZE, depthBits);
        if (stencilBits > 0)
            Add(at, EGL_STENCIL_SIZE, stencilBits);
    }

#ifdef EGL_COLOR_COMPONENT_TYPE_EXT
    if (wantFloatComponents) {
        // Requires EGL_EXT_pixel_format_float
        Add(at, EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT);
    }
#endif

    Add(at, EGL_CONFIG_CAVEAT, EGL_NONE);
    if (msaaSamples > 1) {
        Add(at, EGL_SAMPLES, (EGLint)msaaSamples);
        Add(at, EGL_SAMPLE_BUFFERS, 1);
    }
    at.push_back(EGL_NONE);
    at.push_back(EGL_NONE);

    return TryChoose(dpy, at.data());
}

EGLConfig PickFirstWindowConfig(EGLDisplay dpy, const BackbufferReq& req, DesiredOutput desired,
    bool havePixelFloatExt /* EGL_EXT_pixel_format_float */)
{
    switch (desired) {
        case DesiredOutput::HDR_SCRGB_F16:
            if (havePixelFloatExt) {
                if (auto c = TryPattern(
                    dpy, req, 16, 16, 16, 16, req.minDepthBits, req.minStencilBits, true, req.msaaSamples))
                    return c;
            }
            // fall-through
        case DesiredOutput::HDR_1010102: {
            // 10:10:10:2
            if (auto c =
                    TryPattern(dpy, req, 10, 10, 10, 2, req.minDepthBits, req.minStencilBits, false, req.msaaSamples))
                return c;
            // fall-through
        }
        case DesiredOutput::SDR_SRGB:
        case DesiredOutput::SDR_LINEAR: {
            // Prefer 8:8:8:8
            if (auto c =
                    TryPattern(dpy, req, 8, 8, 8, 8, req.minDepthBits, req.minStencilBits, false, req.msaaSamples)) {
                return c;
            }
            break;
        }
    }
    return EGL_NO_CONFIG_KHR;
}

bool EGLState::CreateContext(DeviceCreateInfo const& createInfo)
{
    auto backendConfig = static_cast<const BackendExtraGLES*>(createInfo.backendConfiguration);
    EGLint major = 0;
    EGLint minor = 0;

    plat_.display = eglGetDisplay(backendConfig ? backendConfig->display : EGL_DEFAULT_DISPLAY);
    const EGLContext appContext = backendConfig ? backendConfig->applicationContext : EGL_NO_CONTEXT;
    const EGLContext sharedContext = backendConfig ? backendConfig->sharedContext : EGL_NO_CONTEXT;
    if (appContext == EGL_NO_CONTEXT && sharedContext == EGL_NO_CONTEXT) {
        EGLint numConfig = 0;
        if (!eglGetConfigs(plat_.display, nullptr, 0, &numConfig) && eglGetError() == EGL_NOT_INITIALIZED) {
            if (!eglInitialize(plat_.display, &major, &minor)) {
                PLUGIN_LOG_E("EGL initialization failed");
                CHECK_EGL_ERROR();
                PLUGIN_ASSERT(false);
                return false;
            }
            plat_.eglInitialized = true;
        }
    }
    if (!major) {
        // Check version from display as we may not have called eglInitialize ourselves
        const string_view version = eglQueryString(plat_.display, EGL_VERSION);
        if (!version.empty()) {
            ParseMajorAndMinor(version, major, minor);
        } else {
            PLUGIN_LOG_F("Could not get EGL version.");
            CHECK_EGL_ERROR();
            if (plat_.eglInitialized) {
                eglTerminate(plat_.display);
            }
            return false;
        }
    }
    plat_.majorVersion = static_cast<uint32_t>(major);
    plat_.minorVersion = static_cast<uint32_t>(minor);
    PLUGIN_LOG_I("EGL %d.%d Initialized", major, minor);

    if (!IsVersionGreaterOrEqual(1, 4)) { // 1, 4: egl version
        // we need at least egl 1.4
        PLUGIN_LOG_F("EGL version too old. 1.4 or later requried.");
        if (plat_.eglInitialized) {
            eglTerminate(plat_.display);
        }
        return false;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    CHECK_EGL_ERROR();

    DumpEGLStrings(plat_.display);

    HandleExtensions();

    plat_.hasColorSpaceExt = hasColorSpaceExt_ = HasExtension("EGL_KHR_gl_colorspace");
    hasConfiglessExt_ = HasExtension("EGL_KHR_no_config_context");
    hasSurfacelessExt_ = HasExtension("EGL_KHR_surfaceless_context");
    // we need a config for the context..
    if (!hasConfiglessExt_) {
        ChooseConfiguration(backendConfig);
    }
    if (appContext) {
        // use applications context
        PLUGIN_LOG_I("Using application context in DeviceGLES");
        plat_.context = appContext;
    } else {
        // Create a new context
        CreateContext(backendConfig);
        plat_.contextCreated = true;
    }
    if (plat_.context == EGL_NO_CONTEXT) {
        // we have failed then.
        if (plat_.eglInitialized) {
            eglTerminate(plat_.display);
        }
        return false;
    }

    if (!hasSurfacelessExt_) {
        // Create a placeholder pbuffer, since we do NOT have a surface yet.
        if (plat_.config == EGL_NO_CONFIG_KHR) {
            // we need to choose a config for the surface..
            ChooseConfiguration(backendConfig);
        }
        GLint surface_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        dummySurface_ = eglCreatePbufferSurface(plat_.display, plat_.config, surface_attribs);
        CHECK_EGL_ERROR();
#if RENDER_GL_DEBUG
        DumpEGLSurface(plat_.display, dummySurface_);
#endif
    } else {
        // Surfaceless path
        dummySurface_ = EGL_NO_SURFACE;
    }

    dummyContext_.context = plat_.context;
    dummyContext_.drawSurface = dummyContext_.readSurface = dummySurface_;
    dummyContext_.display = plat_.display;

    if (!VerifyVersion()) {
        if (plat_.contextCreated) {
            eglDestroyContext(plat_.display, plat_.context);
            plat_.context = EGL_NO_CONTEXT;
        }
        if (plat_.eglInitialized) {
            eglTerminate(plat_.display);
        }
        return false;
    }
    return true;
}

void EGLState::GlInitialize()
{
#define getter(a, b) b = reinterpret_cast<a>(eglGetProcAddress(#b))

#define declare(a, b) getter(a, b)
// NOTE: intentional re-include of gl_functions.h
#undef GLES_FUNCTIONS_H
#include "gles/gl_functions.h"
// NOTE: intentional re-include of egl_functions.h
#define declare(a, b) getter(a, b)
#define declare_plat(a, b) ((void)0)
#undef EGL_FUNCTIONS_H
#include "gles/egl_functions.h"
    PlatformInitialize();

    if (!HasExtension("EGL_KHR_image_base")) {
        eglCreateImageKHR = nullptr;
        eglDestroyImageKHR = nullptr;
        PLUGIN_LOG_E("Missing eglCreateImageKHR");
        PLUGIN_LOG_E("Missing eglDestroyImageKHR");
    }

    plat_.deviceName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    plat_.driverVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    BASE_NS::ClearToValue(
        &plat_.deviceProperties, sizeof(plat_.deviceProperties), 0x00, sizeof(plat_.deviceProperties));
    FillProperties(plat_.deviceProperties);

    SetSwapInterval(1); // default to vsync enabled.
}

bool EGLState::IsValid() const
{
    return (plat_.context != EGL_NO_CONTEXT);
}

void EGLState::SaveContext()
{
    PLUGIN_ASSERT(!oldIsSet_);
    oldIsSet_ = true;
    GetContext(oldContext_);
}

void EGLState::SetContext(const SwapchainGLES* swapchain)
{
    if (swapchain == nullptr) {
        SetContext(dummyContext_, true);
    } else {
        ContextState newContext;
        const auto& plat = swapchain->GetPlatformData();
        newContext.display = plat_.display;
        newContext.context = plat_.context;
        newContext.drawSurface = (EGLSurface)plat.surface;
        newContext.readSurface = (EGLSurface)plat.surface;
        SetContext(newContext, false);

        if (vSync_ != plat.vsync) {
            vSync_ = plat.vsync;
            SetSwapInterval(plat.vsync ? 1u : 0u);
        }
    }
}

void EGLState::RestoreContext()
{
    PLUGIN_ASSERT(oldIsSet_);
    SetContext(oldContext_, true);
    oldIsSet_ = false;
}

void EGLState::GetContext(ContextState& state)
{
    state.display = eglGetCurrentDisplay();
    CHECK_EGL_ERROR();
    if (state.display != EGL_NO_DISPLAY) {
        state.context = eglGetCurrentContext();
        CHECK_EGL_ERROR();
        state.readSurface = eglGetCurrentSurface(EGL_READ);
        CHECK_EGL_ERROR();
        state.drawSurface = eglGetCurrentSurface(EGL_DRAW);
        CHECK_EGL_ERROR();
    } else {
        state.context = EGL_NO_CONTEXT;
        state.readSurface = EGL_NO_SURFACE;
        state.drawSurface = EGL_NO_SURFACE;
    }
}

void EGLState::SetContext(const ContextState& state, bool force)
{
    PLUGIN_ASSERT(oldIsSet_);
    if (state.display != EGL_NO_DISPLAY) {
        if ((force) || (oldContext_.display != state.display) || (oldContext_.drawSurface != state.drawSurface) ||
            (oldContext_.readSurface != state.readSurface) || (oldContext_.context != state.context)) {
            if (eglMakeCurrent(state.display, state.drawSurface, state.readSurface, state.context) == EGL_FALSE) {
                CHECK_EGL_ERROR2();
                if (eglMakeCurrent(state.display, dummySurface_, dummySurface_, state.context) == EGL_FALSE) {
                    CHECK_EGL_ERROR2();
                }
            }
        }
    } else {
        // Okay, do nothing.
        // no display was active, so there can be no surface and no context.
        // We need a display to deactivate context. (EGL_NO_DISPLAY is not a valid argument to eglMakeCurrent)
        // so what to do, leak context?
        // Or just disconnect context/surface from plat_.display (which currently is EGL_DEFAULT_DISPLAY...)
        if (eglMakeCurrent(plat_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
            CHECK_EGL_ERROR2();
        }
    }
}

void EGLState::DestroyContext()
{
    if (plat_.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(plat_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        CHECK_EGL_ERROR();
        if (dummySurface_ != EGL_NO_SURFACE) {
            eglDestroySurface(plat_.display, dummySurface_);
            dummySurface_ = EGL_NO_SURFACE;
        }
        CHECK_EGL_ERROR();
        if (plat_.contextCreated) {
            plat_.contextCreated = false;
            if (plat_.context != EGL_NO_CONTEXT) {
                eglDestroyContext(plat_.display, plat_.context);
                CHECK_EGL_ERROR();
                plat_.context = EGL_NO_CONTEXT;
            }
        }
        if (plat_.eglInitialized) {
            plat_.eglInitialized = false;
            eglTerminate(plat_.display);
            CHECK_EGL_ERROR();
        }
    }
}

bool EGLState::HasExtension(const string_view extension) const
{
    for (const auto& e : dextensionList_) {
        if (extension == e)
            return true;
    }
    for (const auto& e : cextensionList_) {
        if (extension == e)
            return true;
    }
    return false;
}

void EGLState::SetSwapInterval(uint32_t interval)
{
    eglSwapInterval(plat_.display, (EGLint)interval);
}

const DevicePlatformData& EGLState::GetPlatformData() const
{
    return plat_;
}

void* EGLState::ErrorFilter() const
{
    return reinterpret_cast<void*>(FilterError);
}

void EGLState::EnsureConfigSelected(uint32_t flags) const noexcept
{
    if (plat_.config == EGL_NO_CONFIG_KHR) {
        DesiredOutput desired;
        if ((flags & SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT)) {
            if ((flags & SwapchainFlagBits::CORE_SWAPCHAIN_HDR_BIT)) {
                desired = DesiredOutput::HDR_1010102;
            } else {
                desired = DesiredOutput::SDR_SRGB;
            }
        } else {
            if ((flags & SwapchainFlagBits::CORE_SWAPCHAIN_HDR_BIT)) {
                desired = DesiredOutput::HDR_SCRGB_F16;
            } else {
                desired = DesiredOutput::SDR_LINEAR;
            }
        }
        if (plat_.config == EGL_NO_CONFIG_KHR) {
            BackbufferReq req {};
            req.needDefaultDepthStencil = flags & SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT;
            req.minDepthBits = 24;
            req.minStencilBits = 8;
            req.msaaSamples = 0;
            const bool havePixelFloat = HasExtension("EGL_EXT_pixel_format_float");
            const_cast<EGLState*>(this)->plat_.config =
                PickFirstWindowConfig(plat_.display, req, desired, havePixelFloat);
            if (plat_.config == EGL_NO_CONFIG_KHR) {
                PLUGIN_LOG_E("No matching EGLConfig for requested output");
            }
        }
    }
}

uintptr_t EGLState::CreateSurface(uintptr_t window, uintptr_t /* instance */, uint32_t flags) const noexcept
{
    // Check if sRGB colorspace is supported by EGL.
    const bool isSurfaceColorspaceSupported = IsSurfaceColorspaceSupported(plat_);
    EnsureConfigSelected(flags);
    EGLint attribsSrgb[] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (isSurfaceColorspaceSupported) {
        if (IsVersionGreaterOrEqual(1, 5)) { // 1, 5: egl version
            attribsSrgb[0] = EGL_GL_COLORSPACE;
            attribsSrgb[1] = (flags & SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT) ? EGL_GL_COLORSPACE_SRGB
                                                                                  : EGL_GL_COLORSPACE_LINEAR;
        } else if (hasColorSpaceExt_) {
            attribsSrgb[0] = EGL_GL_COLORSPACE_KHR;
            attribsSrgb[1] = (flags & SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT) ? EGL_GL_COLORSPACE_SRGB_KHR
                                                                                  : EGL_GL_COLORSPACE_LINEAR_KHR;
        }
    }
    // depending on the platform EGLNativeWindowType may be a pointer, uintptr_t or khronos_uintptr_t or....
    EGLNativeWindowType eglWindow = reinterpret_cast<EGLNativeWindowType>(reinterpret_cast<void*>(window));
    // Check if config is headless only
    GLint surfaceBits = 0;
    if (EGL_TRUE == eglGetConfigAttrib(plat_.display, plat_.config, EGL_SURFACE_TYPE, &surfaceBits)) {
        if (!(surfaceBits & EGL_WINDOW_BIT)) {
            PLUGIN_LOG_E("EGL config is headless: EGL_WINDOW_BIT missing from config attribute EGL_SURFACE_TYPE.");
        }
    }
    EGLSurface eglSurface = eglCreateWindowSurface(
        plat_.display, plat_.config, eglWindow, isSurfaceColorspaceSupported ? attribsSrgb : nullptr);
    if (eglSurface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        PLUGIN_LOG_E("eglCreateWindowSurface failed (with null attributes): %d", error);
    }
    return reinterpret_cast<uintptr_t>(eglSurface);
}

void EGLState::DestroySurface(uintptr_t surface) const noexcept
{
    if (reinterpret_cast<EGLSurface>(surface) != EGL_NO_SURFACE) {
        eglDestroySurface(plat_.display, reinterpret_cast<EGLSurface>(surface));
    }
}

bool EGLState::GetSurfaceInformation(
    const DevicePlatformDataGLES& plat, EGLSurface surface, GlesImplementation::SurfaceInfo& res) const
{
    EGLDisplay display = plat.display;

#ifndef NDEBUG
    PLUGIN_LOG_V("EGLState::GetSurfaceInformation: input surface information:");
    DumpEGLSurface(display, surface);
#endif
    EGLint configId;
    // Get configId from surface
    if (eglQuerySurface(display, surface, EGL_CONFIG_ID, &configId) == false) {
        PLUGIN_LOG_E("EGLState::GetSurfaceInformation: Could not fetch surface config_id.");
        return false;
    }

    EGLConfig config = EGL_NO_CONFIG_KHR;
    EGLint numconfigs = 0;
    EGLint attrs[] = { EGL_CONFIG_ID, configId, EGL_NONE };
    if (eglChooseConfig(display, attrs, &config, 1, &numconfigs) == false) {
        PLUGIN_LOG_E("EGLState::GetSurfaceInformation: Could not fetch surface config.");
        return false;
    }

    PLUGIN_LOG_V("EGLState::GetSurfaceInformation: input surface configuration:");
    DumpEGLConfig(display, config);

#ifndef NDEBUG
    if (!hasConfiglessExt_) {
        // Check that it matches the config id from "system config"
        EGLConfig plat_config = plat.config;
        if ((plat_config != EGL_NO_CONFIG_KHR) && (plat_config != config)) {
            PLUGIN_ASSERT_MSG(plat_config == config, "display config and surface config should match!");
            PLUGIN_LOG_V("EGLState::GetSurfaceInformation: plat surface configuration:");
            EGLHelpers::DumpEGLConfig(display, plat_config);
        }
    }
#endif
    FillSurfaceInfo(display, surface, configId, config, res);
    return true;
}

void EGLState::SwapBuffers(const SwapchainGLES& swapchain)
{
    SetContext(&swapchain);
    const auto& platSwapchain = static_cast<const SwapchainPlatformDataGL&>(swapchain.GetPlatformData());
    eglSwapBuffers(plat_.display, (EGLSurface)platSwapchain.surface);
}
} // namespace EGLHelpers
RENDER_END_NAMESPACE()
#endif
