/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef declare
// clang-format off
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
// clang-format on
#define declare(a, b) extern a b;
#endif

#if defined(_WIN32)
// The following are gles 3.2 core, but need to be fetched with eglGetProcAddress on PowerVR and MALI simulators.
declare(PFNGLDRAWELEMENTSBASEVERTEXPROC, glDrawElementsBaseVertex);
declare(PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC, glDrawElementsInstancedBaseVertex);
declare(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback);
declare(PFNGLDEBUGMESSAGECONTROLPROC, glDebugMessageControl);
declare(PFNGLPUSHDEBUGGROUPPROC, glPushDebugGroup);
declare(PFNGLPOPDEBUGGROUPPROC, glPopDebugGroup);
declare(PFNGLCOLORMASKIPROC, glColorMaski);
declare(PFNGLENABLEIPROC, glEnablei);
declare(PFNGLDISABLEIPROC, glDisablei);
declare(PFNGLBLENDFUNCSEPARATEIPROC, glBlendFuncSeparatei);
declare(PFNGLBLENDEQUATIONSEPARATEIPROC, glBlendEquationSeparatei);
declare(PFNGLREADNPIXELSPROC, glReadnPixels)
#endif

// define GL_EXT_buffer_storage if not defined in gl headers.
#ifndef GL_EXT_buffer_storage
#define GL_EXT_buffer_storage 1
#ifndef GL_MAP_READ_BIT
const unsigned int GL_MAP_READ_BIT = 0x0001;
#endif
#ifndef GL_MAP_WRITE_BIT
const unsigned int GL_MAP_WRITE_BIT = 0x0002;
#endif
const unsigned int GL_MAP_PERSISTENT_BIT_EXT = 0x0040;
const unsigned int GL_MAP_COHERENT_BIT_EXT = 0x0080;
const unsigned int GL_DYNAMIC_STORAGE_BIT_EXT = 0x0100;
const unsigned int GL_CLIENT_STORAGE_BIT_EXT = 0x0200;
const unsigned int GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT = 0x00004000;
const unsigned int GL_BUFFER_IMMUTABLE_STORAGE_EXT = 0x821F;
const unsigned int GL_BUFFER_STORAGE_FLAGS_EXT = 0x8220;
typedef void(GL_APIENTRYP PFNGLBUFFERSTORAGEEXTPROC)(
    GLenum target, GLsizeiptr size, const void* data, GLbitfield flags);
#endif

// We hope to use GL_EXT_buffer_storage
declare(PFNGLBUFFERSTORAGEEXTPROC, glBufferStorageEXT);

// define GL_OES_EGL_image if not defined in gl headers.
#ifndef GL_OES_EGL_image
#define GL_OES_EGL_image
using GLeglImageOES = void*;
using PFNGLEGLIMAGETARGETTEXTURE2DOESPROC = void(GL_APIENTRYP*)(GLenum target, GLeglImageOES image);
#endif /* GL_OES_EGL_image */

declare(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC, glEGLImageTargetTexture2DOES);

#ifndef GL_EXT_multisampled_render_to_texture
#define GL_EXT_multisampled_render_to_texture
const unsigned int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT = 0x8D6C;
const unsigned int GL_RENDERBUFFER_SAMPLES_EXT = 0x8CAB;
const unsigned int GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT = 0x8D56;
const unsigned int GL_MAX_SAMPLES_EXT 0x8D57;
using PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC = void(GL_APIENTRYP)(
    GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
using PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC = void(GL_APIENTRYP )(
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif /* GL_EXT_multisampled_render_to_texture */

declare(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC, glRenderbufferStorageMultisampleEXT);
declare(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC, glFramebufferTexture2DMultisampleEXT);

#ifndef GL_EXT_disjoint_timer_query
#define GL_EXT_disjoint_timer_query 1
#define GL_QUERY_COUNTER_BITS_EXT 0x8864
#define GL_CURRENT_QUERY_EXT 0x8865
#define GL_QUERY_RESULT_EXT 0x8866
#define GL_QUERY_RESULT_AVAILABLE_EXT 0x8867
#define GL_TIME_ELAPSED_EXT 0x88BF
#define GL_TIMESTAMP_EXT 0x8E28
#define GL_GPU_DISJOINT_EXT 0x8FBB

using PFNGLGETQUERYOBJECTUI64VEXTPROC = void(GL_APIENTRYP)(GLuint id, GLenum pname, GLuint64* params);
#endif /* GL_EXT_disjoint_timer_query */

declare(PFNGLGETQUERYOBJECTUI64VEXTPROC, glGetQueryObjectui64vEXT);
#elif RENDER_HAS_GL_BACKEND

#ifndef declare
#include <gl/glcorearb.h>
#include <gl/glext.h>
#define declare(a, b) extern a b;
#endif

declare(PFNGLDRAWBUFFERSPROC, glDrawBuffers);
declare(PFNGLPUSHDEBUGGROUPPROC, glPushDebugGroup);
declare(PFNGLPOPDEBUGGROUPPROC, glPopDebugGroup);
declare(PFNGLACTIVETEXTUREPROC, glActiveTexture);
declare(PFNGLATTACHSHADERPROC, glAttachShader);
declare(PFNGLBINDBUFFERPROC, glBindBuffer);
declare(PFNGLBINDBUFFERRANGEPROC, glBindBufferRange);
declare(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
declare(PFNGLBINDIMAGETEXTUREPROC, glBindImageTexture);
declare(PFNGLBINDSAMPLERPROC, glBindSampler);
declare(PFNGLBINDTEXTUREPROC, glBindTexture);
declare(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
declare(PFNGLBINDVERTEXBUFFERPROC, glBindVertexBuffer);
declare(PFNGLVERTEXBINDINGDIVISORPROC, glVertexBindingDivisor);
declare(PFNGLBLENDCOLORPROC, glBlendColor);
declare(PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate);
declare(PFNGLBLENDEQUATIONSEPARATEIPROC, glBlendEquationSeparatei);
declare(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
declare(PFNGLBLENDFUNCSEPARATEIPROC, glBlendFuncSeparatei);
declare(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer);
declare(PFNGLBUFFERDATAPROC, glBufferData);
declare(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
declare(PFNGLCLEARBUFFERFIPROC, glClearBufferfi);
declare(PFNGLCLEARBUFFERFVPROC, glClearBufferfv);
declare(PFNGLCLEARBUFFERIVPROC, glClearBufferiv);
declare(PFNGLCOLORMASKPROC, glColorMask);
declare(PFNGLCOLORMASKIPROC, glColorMaski);
declare(PFNGLCOMPILESHADERPROC, glCompileShader);
declare(PFNGLREADPIXELSPROC, glReadPixels);
declare(PFNGLREADNPIXELSPROC, glReadnPixels);
declare(PFNGLCOPYBUFFERSUBDATAPROC, glCopyBufferSubData);
declare(PFNGLCREATEPROGRAMPROC, glCreateProgram);
declare(PFNGLCREATESHADERPROC, glCreateShader);
declare(PFNGLPROGRAMUNIFORM1IPROC, glProgramUniform1i);
declare(PFNGLPROGRAMUNIFORM1IVPROC, glProgramUniform1iv);
declare(PFNGLPROGRAMUNIFORM1UIVPROC, glProgramUniform1uiv);
declare(PFNGLPROGRAMUNIFORM4UIVPROC, glProgramUniform4uiv);
declare(PFNGLPROGRAMUNIFORM1FVPROC, glProgramUniform1fv);
declare(PFNGLPROGRAMUNIFORM2FVPROC, glProgramUniform2fv);
declare(PFNGLPROGRAMUNIFORM4FVPROC, glProgramUniform4fv);
declare(PFNGLPROGRAMUNIFORMMATRIX4FVPROC, glProgramUniformMatrix4fv);
declare(PFNGLCULLFACEPROC, glCullFace);
declare(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback);
declare(PFNGLDEBUGMESSAGECONTROLPROC, glDebugMessageControl);
declare(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
declare(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
declare(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
declare(PFNGLDELETESAMPLERSPROC, glDeleteSamplers);
declare(PFNGLDELETESHADERPROC, glDeleteShader);
declare(PFNGLDELETETEXTURESPROC, glDeleteTextures);
declare(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
declare(PFNGLDEPTHFUNCPROC, glDepthFunc);
declare(PFNGLDEPTHMASKPROC, glDepthMask);
declare(PFNGLDEPTHRANGEFPROC, glDepthRangef);
declare(PFNGLDISABLEPROC, glDisable);
declare(PFNGLDISABLEIPROC, glDisablei);
declare(PFNGLDISPATCHCOMPUTEPROC, glDispatchCompute);
declare(PFNGLDISPATCHCOMPUTEINDIRECTPROC, glDispatchComputeIndirect);
declare(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced);
declare(PFNGLDRAWARRAYSINDIRECTPROC, glDrawArraysIndirect);
declare(PFNGLDRAWARRAYSPROC, glDrawArrays);
declare(PFNGLDRAWELEMENTSBASEVERTEXPROC, glDrawElementsBaseVertex);
declare(PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC, glDrawElementsInstancedBaseVertex);
declare(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced);
declare(PFNGLDRAWELEMENTSINDIRECTPROC, glDrawElementsIndirect);
declare(PFNGLDRAWELEMENTSPROC, glDrawElements);
declare(PFNGLENABLEPROC, glEnable);
declare(PFNGLENABLEIPROC, glEnablei);
declare(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
declare(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
declare(PFNGLFRAMEBUFFERTEXTURELAYERPROC, glFramebufferTextureLayer);
declare(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
declare(PFNGLFRONTFACEPROC, glFrontFace);
declare(PFNGLGENBUFFERSPROC, glGenBuffers);
declare(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
declare(PFNGLGENSAMPLERSPROC, glGenSamplers);
declare(PFNGLGENTEXTURESPROC, glGenTextures);
declare(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
declare(PFNGLGETSTRINGPROC, glGetString);
declare(PFNGLGETSTRINGIPROC, glGetStringi);
declare(PFNGLGETFLOATVPROC, glGetFloatv);
declare(PFNGLGETINTEGERVPROC, glGetIntegerv);
declare(PFNGLGETINTEGERI_VPROC, glGetIntegeri_v);
declare(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
declare(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
declare(PFNGLGETACTIVEUNIFORMBLOCKIVPROC, glGetActiveUniformBlockiv);
declare(PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC, glGetActiveUniformBlockName);
declare(PFNGLUNIFORMBLOCKBINDINGPROC, glUniformBlockBinding);
declare(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
declare(PFNGLGETSHADERIVPROC, glGetShaderiv);
declare(PFNGLGETUNIFORMIVPROC, glGetUniformiv);
declare(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
declare(PFNGLGETACTIVEUNIFORMPROC, glGetActiveUniform);
declare(PFNGLGETPROGRAMINTERFACEIVPROC, glGetProgramInterfaceiv);
declare(PFNGLGETPROGRAMRESOURCENAMEPROC, glGetProgramResourceName);
declare(PFNGLGETPROGRAMRESOURCEINDEXPROC, glGetProgramResourceIndex);
declare(PFNGLGETPROGRAMRESOURCEIVPROC, glGetProgramResourceiv);
declare(PFNGLLINEWIDTHPROC, glLineWidth);
declare(PFNGLLINKPROGRAMPROC, glLinkProgram);
declare(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange);
declare(PFNGLMEMORYBARRIERPROC, glMemoryBarrier);
declare(PFNGLMEMORYBARRIERBYREGIONPROC, glMemoryBarrierByRegion);
declare(PFNGLPIXELSTOREIPROC, glPixelStorei);
declare(PFNGLPOLYGONMODEPROC, glPolygonMode);
declare(PFNGLPOLYGONOFFSETPROC, glPolygonOffset);
declare(PFNGLSAMPLERPARAMETERFPROC, glSamplerParameterf);
declare(PFNGLSAMPLERPARAMETERFVPROC, glSamplerParameterfv);
declare(PFNGLSAMPLERPARAMETERIPROC, glSamplerParameteri);
declare(PFNGLSCISSORPROC, glScissor);
declare(PFNGLSHADERSOURCEPROC, glShaderSource);
declare(PFNGLSTENCILFUNCSEPARATEPROC, glStencilFuncSeparate);
declare(PFNGLSTENCILMASKSEPARATEPROC, glStencilMaskSeparate);
declare(PFNGLSTENCILOPSEPARATEPROC, glStencilOpSeparate);
declare(PFNGLTEXPARAMETERIPROC, glTexParameteri);
declare(PFNGLTEXSTORAGE2DPROC, glTexStorage2D);
declare(PFNGLTEXSTORAGE3DPROC, glTexStorage3D);
declare(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
declare(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D);
declare(PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC, glCompressedTexSubImage2D);
declare(PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC, glCompressedTexSubImage3D);
declare(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
declare(PFNGLUSEPROGRAMPROC, glUseProgram);
declare(PFNGLVERTEXATTRIBBINDINGPROC, glVertexAttribBinding);
declare(PFNGLVERTEXATTRIBFORMATPROC, glVertexAttribFormat);
declare(PFNGLVERTEXATTRIBIFORMATPROC, glVertexAttribIFormat);
declare(PFNGLVIEWPORTPROC, glViewport);
declare(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync);
declare(PFNGLDELETESYNCPROC, glDeleteSync);
declare(PFNGLFENCESYNCPROC, glFenceSync);
declare(PFNGLBUFFERSTORAGEPROC, glBufferStorage);

declare(PFNGLTEXSTORAGE2DMULTISAMPLEPROC, glTexStorage2DMultisample);

declare(PFNGLGENQUERIESPROC, glGenQueries);
declare(PFNGLDELETEQUERIESPROC, glDeleteQueries);
declare(PFNGLBEGINQUERYPROC, glBeginQuery);
declare(PFNGLENDQUERYPROC, glEndQuery);
declare(PFNGLGETQUERYOBJECTUIVPROC, glGetQueryObjectuiv);
declare(PFNGLGETQUERYOBJECTUI64VPROC, glGetQueryObjectui64v);

declare(PFNGLINVALIDATEFRAMEBUFFERPROC, glInvalidateFramebuffer);
declare(PFNGLINVALIDATESUBFRAMEBUFFERPROC, glInvalidateSubFramebuffer);
declare(PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC, glGetFramebufferAttachmentParameteriv);

declare(PFNGLFINISHPROC, glFinish);
declare(PFNGLFLUSHPROC, glFlush);

declare(PFNGLGENPROGRAMPIPELINESPROC, glGenProgramPipelines);
declare(PFNGLBINDPROGRAMPIPELINEPROC, glBindProgramPipeline);
declare(PFNGLUSEPROGRAMSTAGESPROC, glUseProgramStages);
declare(PFNGLDELETEPROGRAMPIPELINESPROC, glDeleteProgramPipelines);
declare(PFNGLPROGRAMPARAMETERIPROC, glProgramParameteri);
declare(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv);

declare(PFNGLGETBUFFERPARAMETERIVPROC, glGetBufferParameteriv);
declare(PFNGLGETBUFFERPARAMETERI64VPROC, glGetBufferParameteri64v);
declare(PFNGLGETBUFFERPOINTERVPROC, glGetBufferPointerv);

declare(PFNGLOBJECTLABELPROC, glObjectLabel);

// Define the EXT versions, to match GLES.
#define glBufferStorageEXT glBufferStorage
#define GL_MAP_PERSISTENT_BIT_EXT GL_MAP_PERSISTENT_BIT
#define GL_MAP_COHERENT_BIT_EXT GL_MAP_COHERENT_BIT
#define GL_DYNAMIC_STORAGE_BIT_EXT GL_DYNAMIC_STORAGE_BIT
#define GL_CLIENT_STORAGE_BIT_EXT GL_CLIENT_STORAGE_BIT

#ifndef GL_EXT_timer_query
// GL_GPU_DISJOINT_EXT does not exist and is ignored.
#define GL_TIME_ELAPSED_EXT GL_TIME_ELAPSED
#endif

declare(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
declare(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
declare(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
declare(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
declare(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glRenderbufferStorageMultisample);
declare(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers);
#else
#pragma(error, "Neither GL or GLES is enabled in GLFunctions.h")
#endif
#undef declare