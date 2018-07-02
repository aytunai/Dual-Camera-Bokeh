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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#if defined(__XTENSA__)
#include <sys/times.h>
#include <xtensa/sim.h>
#include <xtensa/tie/xt_ivpn.h>
#endif

#include "xi_api.h"
#include "typedef.h"

/**************************************************************
*
* SETUP_TILE
* (
*     ptile,    Tile pointer
*     buf,      Tile buffer start address
*     actvbuf,  Tile buffer start address of active data
*     width,    Tile width
*     height,   Tile height
*     pitch,    Tile pitch
*     bufsz,    Size of tile in bytes
*     type,     Tile type
*     edgew,    Tile edge width
*     edgeh,    Tile edge height
*     XCoord,   Tile X coordinate in frame
*     YCoord    Tile Y coordinate in frame
* )
*
**************************************************************/

#define  SETUP_ARRAY(pArray, buf, actvbuf, width, height, pitch, bufsz, type) \
  XI_ARRAY_SET_BUFF_PTR((pArray), (buf));                                     \
  XI_ARRAY_SET_DATA_PTR((pArray), (actvbuf));                                 \
  XI_ARRAY_SET_BUFF_SIZE((pArray), (bufsz));                                  \
  XI_ARRAY_SET_WIDTH((pArray), (width));                                      \
  XI_ARRAY_SET_HEIGHT((pArray), (height));                                    \
  XI_ARRAY_SET_PITCH((pArray), (pitch));                                      \
  XI_ARRAY_SET_TYPE((pArray), (type));                                        \

_XI_EXTERN_C_ PPMImage *readPPM(const char *filename);
_XI_EXTERN_C_ void extractRGB8(uint8_t *dataptr, uint8_t *srcR, uint8_t *srcG, uint8_t *srcB, int16_t srcWidth, int16_t srcHeight);
_XI_EXTERN_C_ void extractRGB8toGrey(uint8_t *dataptr, uint8_t *src, int16_t srcWidth, int16_t srcHeight);
_XI_EXTERN_C_ void fillRGB8(uint8_t *dataptr, uint8_t *srcR, uint8_t *srcG, uint8_t *srcB, int16_t srcWidth, int16_t srcHeight);
_XI_EXTERN_C_ void writePPM(const char *filename, PPMImage *img);
_XI_EXTERN_C_ void extractTILE(uint8_t *srcImg, uint8_t *roiImg, int32_t srcPitch, int32_t srcHeight, int32_t roiW, int32_t roiH, int32_t Xcoord, int32_t Ycoord);
_XI_EXTERN_C_ void moveTile8(uint8_t *src, uint8_t *dst, int32_t srcWidth, int32_t srcHeight, int32_t srcPitch, int32_t dstPitch);
_XI_EXTERN_C_ void moveTile16(uint16_t *src, uint16_t *dst, int32_t srcWidth, int32_t srcHeight, int32_t srcPitch, int32_t dstPitch);
_XI_EXTERN_C_ int32_t compare_u8(uint8_t *out_ptr, uint8_t *ref_ptr, int32_t width, int32_t height, int32_t pitch, int32_t tolerance);
_XI_EXTERN_C_ char compareLines(xi_line_polar_fpt* ref, xi_line_polar_fpt* dst, int count, int errr, int errt);
_XI_EXTERN_C_ void drawLine(xi_line_polar_fpt *lineBuf, uint8_t *dataBuf, xi_size img_size, int cnt);

#if defined(__XTENSA__)
#  include <xtensa/sim.h>
#define TIME_STAMP(cyc_cnt)      \
  __asm__ __volatile__ (         \
    "rsr.ccount %0"              \
    : "=a" (cyc_cnt) :: "memory" \
    );
#endif
