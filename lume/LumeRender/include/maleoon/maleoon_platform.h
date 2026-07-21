// $Copyright
//
// This header is generated from the Maleoon JSON Registry.
//

#pragma once

#ifndef MLN_PLATFORM_H_
#define MLN_PLATFORM_H_

#if !defined(MLN_NO_STDDEF_H)
#include <stddef.h>
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
#include <stdint.h>
#endif

#endif  // !defined(MLN_NO_STDDEF_H)

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/* Platform macros.
 *
 * Platforms should define these so that Maleoon clients call Maleoon commands
 * with the same calling conventions that the Maleoon implementation expects.
 *
 * MLNAPI_ATTR - Placed before the return type in function declarations.
 *               Useful for C++11 and GCC/Clang-style function attribute syntax.
 * MLNAPI_CALL - Placed after the return type in function declarations.
 *               Useful for MSVC-style calling convention syntax.
 * MLNAPI_PTR  - Placed between the '(' and '*' in function pointer types.
 *
 * Function declaration:  MLNAPI_ATTR void MLNAPI_CALL mlnCommand(void);
 * Function pointer type: typedef void (MLNAPI_PTR *PFN_mlnCommand)(void);
 */
#define MLNAPI_ATTR
#define MLNAPI_CALL
#define MLNAPI_PTR

#if !defined(MLN_NO_STDINT_H)
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
typedef unsigned __int32 uint32_t;
typedef signed __int32 int32_t;
typedef unsigned __int16 uint16_t;
typedef signed __int16 int16_t;
typedef unsigned __int8 uint8_t;
typedef signed __int8 int8_t;
#else
#include <stdint.h>
#endif
#endif  // !defined(MLN_NO_STDINT_H)

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif
