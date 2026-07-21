// $Copyright
//
// This header is generated from the Maleoon JSON Registry.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MLN_DEFINE_HANDLE(object) typedef struct object##_T* object

#ifndef MLN_USE_64_BIT_PTR_DEFINES
#if defined(__LP64__) || defined(_WIN64) ||                            \
    (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) || \
    defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||     \
    defined(__powerpc64__) || (defined(__riscv) && __riscv_xlen == 64)
#define MLN_USE_64_BIT_PTR_DEFINES 1
#else
#define MLN_USE_64_BIT_PTR_DEFINES 0
#endif
#endif

#ifndef MLN_DEFINE_NON_DISPATCHABLE_HANDLE
#if (MLN_USE_64_BIT_PTR_DEFINES == 1)
#if (defined(__cplusplus) && (__cplusplus >= 201103L)) || \
    (defined(_MSVC_LANG) && (_MSVC_LANG >= 201103L))
#define MLN_NULL_HANDLE nullptr
#else
#define MLN_NULL_HANDLE ((void*)0)
#endif
#else
#define MLN_NULL_HANDLE 0ULL
#endif
#endif
#ifndef MLN_NULL_HANDLE
#define MLN_NULL_HANDLE 0
#endif

#ifndef MLN_DEFINE_NON_DISPATCHABLE_HANDLE
#if (MLN_USE_64_BIT_PTR_DEFINES == 1)
#define MLN_DEFINE_NON_DISPATCHABLE_HANDLE(object) \
    typedef struct object##_T* object
#else
#define MLN_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object
#endif
#endif

#define MLN_DEFINE_HANDLE(object) typedef struct object##_T* object

#ifndef MLN_USE_64_BIT_PTR_DEFINES
#if defined(__LP64__) || defined(_WIN64) ||                            \
    (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) || \
    defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||     \
    defined(__powerpc64__) || (defined(__riscv) && __riscv_xlen == 64)
#define MLN_USE_64_BIT_PTR_DEFINES 1
#else
#define MLN_USE_64_BIT_PTR_DEFINES 0
#endif
#endif

#ifndef MLN_DEFINE_NON_DISPATCHABLE_HANDLE
#if (MLN_USE_64_BIT_PTR_DEFINES == 1)
#if (defined(__cplusplus) && (__cplusplus >= 201103L))
#define MLN_NULL_HANDLE nullptr
#else
#define MLN_NULL_HANDLE ((void*)0)
#endif
#else
#define MLN_NULL_HANDLE 0ULL
#endif
#endif
#ifndef MLN_NULL_HANDLE
#define MLN_NULL_HANDLE 0
#endif

#ifndef MLN_DEFINE_NON_DISPATCHABLE_HANDLE
#if (MLN_USE_64_BIT_PTR_DEFINES == 1)
#define MLN_DEFINE_NON_DISPATCHABLE_HANDLE(object) \
    typedef struct object##_T* object
#else
#define MLN_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object
#endif
#endif

#define MLN_MAKE_API_VERSION(variant, major, minor, patch)           \
    ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | \
     (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

#define MLN_API_VERSION_VARIANT(version) ((uint32_t)(version) >> 29U)

#define MLN_API_VERSION_MAJOR(version) (((uint32_t)(version) >> 22U) & 0x7FU)

#define MLN_API_VERSION_MINOR(version) (((uint32_t)(version) >> 12U) & 0x3FFU)

#define MLN_API_VERSION_PATCH(version) ((uint32_t)(version)&0xFFFU)

// Maleoon 1.0 version number
#define MLN_API_VERSION_1_0 \
    MLN_MAKE_API_VERSION(0, 1, 0, 0)  // Patch version should always be set to 0

// Version of this file
#define MLN_HEADER_VERSION 0

// Complete version of this file
#define MLN_HEADER_VERSION_COMPLETE \
    MLN_MAKE_API_VERSION(0, 1, 0, MLN_HEADER_VERSION)

// Maleoon API Constants
#define MLN_TRUE (1)
#define MLN_FALSE (0)
#define MLN_LOD_CLAMP_NONE (1000.0F)
#define MLN_REMAINING_ARRAY_LAYERS ((~0U))
#define MLN_REMAINING_MIP_LEVELS ((~0U))
#define MLN_WHOLE_SIZE ((~0ULL))
#define MLN_COMMAND_INDEX_SHARED ((~0U))
#define MLN_MAX_LAYER_NAME_SIZE (256)
#define MLN_MAX_DESCRIPTION_SIZE (256)
#define MLN_UUID_SIZE (16)
#define MLN_MAX_GPU_MEMORY_TYPES (32)
#define MLN_MAX_GPU_MEMORY_HEAPS (16)
#define MLN_MAX_COUNTER_TYPES (8)
#define MLN_MAX_COUNTER_NAME_SIZE (256)
#define MLN_MAX_COUNTER_DESCRIPTION_SIZE (256)
#define MLN_MAX_VALIDATION_LOG_FILE_PATH_SIZE (256)

#ifdef __cplusplus
}
#endif
