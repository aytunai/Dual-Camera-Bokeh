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

/* *****************************************************************************
 * FILE:  typedef.h
 *
 * DESCRIPTION:
 *
 *    This file contains prototype definitions of structures and Macros related to
 *    Affine-Warp routines that are being used/accessed in other project files
 *
 * ****************************************************************************/


#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#define _USE_MATH_DEFINES
#include <math.h>
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_timer.h>
#ifndef M_PI
#define M_PI  XI_PI
#endif

#if defined (__XTENSA__)
#include <xtensa/xt_profiling.h>
#endif

#define INTERRUPT_ON_COMPLETION  0
#define INPUT_TILE_BUF_RATIO     9

#define DMA_DESCR_CNT            32 // number of DMA descriptors

#define EMULATE_LOCAL_RAM  1

#if defined(__XTENSA__) || defined(__GNUC__)
#define ALIGN(x)  __attribute__((aligned(x)))
#define ALIGN16  __attribute__((aligned(16)))
#define ALIGN32  __attribute__((aligned(32)))
#define ALIGN64  __attribute__((aligned(64)))
#else
#define ALIGN(x)  _declspec(align(x))
#define ALIGN16  _declspec(align(16))
#define ALIGN32  _declspec(align(32))
#define ALIGN64  _declspec(align(64))
#define __restrict
#endif

#if EMULATE_LOCAL_RAM && defined(__XTENSA__)
#define _LOCAL_RAM0_  __attribute__((section(".dram0.data")))
#define _LOCAL_RAM1_  __attribute__((section(".dram1.data")))
#else
#define _LOCAL_RAM0_
#define _LOCAL_RAM1_
#endif

#define IVP
//#define HW_PLATFORM

#if defined(__XTENSA__)
#define IVP_SIMD_WIDTH  XCHAL_IVPN_SIMD_WIDTH
#else
#define IVP_SIMD_WIDTH  32
#endif

#if defined(__XTENSA__)
#define TIME_STAMP(cyc_cnt)    \
  {                            \
    cyc_cnt = XT_RSR_CCOUNT(); \
  }

#define USER_DEFINED_HOOKS_START()             \
  {                                            \
    xt_iss_switch_mode(XT_ISS_CYCLE_ACCURATE); \
    xt_iss_trace_level(3);                     \
    xt_iss_client_command("all", "enable");    \
  }

#define USER_DEFINED_HOOKS_STOP()            \
  {                                          \
    xt_iss_switch_mode(XT_ISS_FUNCTIONAL);   \
    xt_iss_trace_level(0);                   \
    xt_iss_client_command("all", "disable"); \
  }

#else //__XTENSA__
#define TIME_STAMP(cyc_cnt) \
  {                         \
    cyc_cnt = 0;            \
  }

#define USER_DEFINED_HOOKS_START()

#define USER_DEFINED_HOOKS_STOP()

#endif //__XTENSA__


typedef struct
{
  unsigned char red, green, blue;
} PPMPixel8;

typedef struct
{
  unsigned short red, green, blue;
} PPMPixel16;

typedef struct
{
  unsigned char Y;
} PPMY8;

typedef struct
{
  unsigned short Y;
} PPMY16;

typedef struct
{
  int  x, y;
  int  compWidth;   // 8 or 16
  void *data;
} PPMImage, PGMImage;

#define CREATOR  "TENSILICA"

#define PRINT_ERROR_LEVEL                                                                                              \
  printf("\nUsing XI Library Version %s.\nError level set to ", XI_LIBRARY_VERSION_STR);                               \
  if (XI_ERROR_LEVEL == 0) {                                                                                           \
    printf("XI_ERROR_LEVEL_NO_ERROR: Error checking disabled.\n"); }                                                   \
  if (XI_ERROR_LEVEL == 1) {                                                                                           \
    printf("XI_ERROR_LEVEL_TERMINATE_ON_ERROR: On error, terminate with call to exit(-1).\n"); }                       \
  if (XI_ERROR_LEVEL == 2) {                                                                                           \
    printf("XI_ERROR_LEVEL_RETURN_ON_ERROR: On error, return error code without any processing.\n"); }                 \
  if (XI_ERROR_LEVEL == 3) {                                                                                           \
    printf("XI_ERROR_LEVEL_CONTINUE_ON_ERROR: On error, return error code but attempt to continue processing.\n"); }   \
  if (XI_ERROR_LEVEL == 4) {                                                                                           \
    printf("XI_ERROR_LEVEL_PRINT_ON_ERROR: On error, print error description and return without any processing.\n"); } \
  if (XI_ERROR_LEVEL == 5) {                                                                                           \
    printf("XI_ERROR_LEVEL_PRINT_AND_CONTINUE_ON_ERROR: On error, print error description but attempt to continue processing.\n"); }


/* XI Library Umbrella Header */
#include "xi_api.h"

typedef struct {
	xvTile *pIn;
	xvTile *pOut;
} TileBuffer;

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
typedef struct {
	xvTileManager tileMgr;
	xi_affine affineOrg;      // original affine matrix
	xi_affine_fpt aFpt;  // decomposed affine matrix
	xi_affine_fpt ainvFpt;  // decomposed affine matrix

	int tileWidth;            // tile width
	int tileHeight;           // tile height
	int numTiles;             // number of tiles in an image segment

	xi_tile srcTile;          // source tile info
	xi_tile dstTile;          // output tile info

	xvFrame *pSrcFrame;     // source image
	xvFrame *pOutFrame;     // output image

	TileBuffer tileBuffer[2];
	xvTile *pTmpTile;

	uint8_t *dramInBuf[2];    // pointer of input double-buffer in DataRAM
	uint8_t *dramOutBuf[2];   // pointer of output double-buffer in DataRAM
	uint16_t *tempBuf;        // pointer of temporary buffer in DataRAM
	xi_array tempArray;       // temporary array used in affine kernel

	// DMA parameters
	int maxPIFBlockSize;
	int numOutstandingPIFReqs;

	// statistics
	uint32_t intrCount;
	uint32_t kernelCycles;
	uint32_t dmaSetupCycles;
	uint32_t dmaBytes;
} Context;

#endif  //__TYPEDEF_H__
