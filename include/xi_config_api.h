/*
 * Copyright (c) 2016 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

#ifndef __XI_CONFIG_API_H__
#define __XI_CONFIG_API_H__

#include "xi_version.h"

#ifdef __XTENSA__
    #include <xtensa/config/core-isa.h>
#else
    #if defined XI_HAVE_CORE_ISA_H
        #include <xtensa/config/core-isa.h>
    #endif
    #if defined(_MSC_VER)
        #pragma warning (disable : 4005 )
    #endif
    #ifdef __cplusplus
        #define restrict  __restrict
    #endif
    #ifndef XCHAL_NUM_DATARAM
        #define XCHAL_NUM_DATARAM  2
    #endif
#endif

#if !defined(__XTENSA__) || !defined(XCHAL_HAVE_VISION) || !XCHAL_HAVE_VISION
#   define XV_EMULATE_DMA
#endif

// #define XI_EMULATE_LOCAL_RAM 0
#ifndef XI_EMULATE_LOCAL_RAM
#  define XI_EMULATE_LOCAL_RAM  1
#endif

/* XI Library API qualifiers */

#if XI_EMULATE_LOCAL_RAM && __XTENSA__
#if XCHAL_NUM_DATARAM == 2
#  define _XI_LOCAL_RAM0_  __attribute__((section(".dram0.data")))
#  define _XI_LOCAL_RAM1_  __attribute__((section(".dram1.data")))
#elif XCHAL_NUM_DATARAM == 1
#  define _XI_LOCAL_RAM0_  __attribute__((section(".dram0.data")))
#endif
#  define _XI_LOCAL_IRAM_  __attribute__((section(".iram0.text")))
#else
#  define _XI_LOCAL_RAM0_
#  define _XI_LOCAL_RAM1_
#  define _XI_LOCAL_IRAM_
#endif

#if defined __GNUC__ && __GNUC__ >= 4
#  define _XI_EXPORTS_  __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#  if defined(XI_CREATE_SHARED_LIBRARY)
#    define _XI_EXPORTS_  __declspec(dllexport)
#  else
#    define _XI_EXPORTS_  __declspec(dllimport)
#  endif
#else
#  define _XI_EXPORTS_
#endif

#ifdef __cplusplus
#  define _XI_EXTERN_C_  extern "C"
#else
#  define _XI_EXTERN_C_  extern
#endif

#ifdef __cplusplus
#  define XI_DEFAULT(value) = (value)
#else
#  define XI_DEFAULT(value)
#endif

#define _XI_API_  _XI_EXTERN_C_ _XI_EXPORTS_

/* data alignment */

#ifndef XI_KEYPOINT_ALIGNMENT
#    define XI_KEYPOINT_ALIGNMENT  1 // can be changed to support 32B alignment for xi_keypoint
#endif

/* error check levels */

/* do not check arguments for errors */
#define XI_ERROR_LEVEL_NO_ERROR                     0
/* call exit(-1) in case of error */
#define XI_ERROR_LEVEL_TERMINATE_ON_ERROR           1
/* return corresponding error code on error without any processing (recommended)*/
#define XI_ERROR_LEVEL_RETURN_ON_ERROR              2
/* capture error but attempt continue processing (dangerous!) */
#define XI_ERROR_LEVEL_CONTINUE_ON_ERROR            3
/* print error message to stdout and return without any processing */
#define XI_ERROR_LEVEL_PRINT_ON_ERROR               4
/* print error message but attempt continue processing (dangerous!) */
#define XI_ERROR_LEVEL_PRINT_AND_CONTINUE_ON_ERROR  5

#ifndef XI_ERROR_LEVEL
#  define XI_ERROR_LEVEL  XI_ERROR_LEVEL_RETURN_ON_ERROR
#endif

#endif
