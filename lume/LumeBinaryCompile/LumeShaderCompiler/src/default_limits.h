#ifndef DEFAULT_LIMITS_H
#define DEFAULT_LIMITS_H

#define GLSLANG_VERSION_11_0_0 110000
#define GLSLANG_VERSION_12_2_0 120200

#if GLSLANG_VERSION > 110000
#include <glslang/Public/ResourceLimits.h>
#else
#include <glslang/Include/ResourceLimits.h>
#endif

extern TBuiltInResource kGLSLangDefaultTResource;

#endif