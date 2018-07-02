/*
 * resize_mask.h
 *
 *  Created on: 2017-9-20
 *      Author: czh3898
 */

#ifndef _RESIZE_MASK_H_
#define _RESIZE_MASK_H_
#if defined(__XTENSA__)
#include <stdbool.h>
#include <sys/times.h>
#include <xtensa/sim.h>
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#endif

/* Image IO utilities */
#include "typedef.h"
#include "tileManager.h"

// Error type code
typedef enum {
	NO_ERROR = 0,
	TM_ERROR,
	XILIB_ERROR,
	MEMORY_ERROR,
	OTHER_ERROR,
} ErrorType;

typedef long					MLong;
typedef float					MFloat;
typedef double					MDouble;
typedef unsigned char			MByte;
typedef unsigned short			MWord;
typedef unsigned int 			MDWord;
typedef void*					MHandle;
typedef char					MChar;
typedef long					MBool;
typedef void					MVoid;
typedef void*					MPVoid;
typedef char*					MPChar;
typedef short					MShort;
typedef const char*				MPCChar;
typedef	MLong					MRESULT;
typedef MDWord					MCOLORREF;
typedef	signed		char		MInt8;
typedef	unsigned	char		MUInt8;
typedef	signed		short		MInt16;
typedef	unsigned	short		MUInt16;
typedef signed		int			MInt32;
typedef unsigned	int			MUInt32;
#define MERR_NONE						0
#define MOK								0

#define MAX(x,y) ((x)<(y) ? (y) : (x))
#define MIN(x,y) ((x)>(y) ? (y) : (x))
#define GLOBAL_Q	(12)
#define _iq(X) 	(int)(X * (1<<GLOBAL_Q))
#define _iqr0(X) (int)((X + (1 << (GLOBAL_Q -1))) >> GLOBAL_Q)

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
typedef struct {
	void *cadenceContext;

	int tileWidth;           	// tile width
	int tileHeight;           	// tile height
	int tilePitch;			  	// tile pitch
	int numTiles;             	// number of tiles in an image segment
	int maxEnlargementFactor; 	// max Enlargement factor
	float		fwratio;		// float point ratio for width
	float		fhratio;		// float point ratio for height
	uint16_t	iq_wratio;		// fixed-point ratio for width
	uint16_t	iq_hratio;		// fixed-point ratio for height


	xi_tile srcTile;          	// source tile info
	xi_tile dstTile;          	// output tile info

	xvFrame *pSrcFrame;     	// source image
	xvFrame *pOutFrame;     	// output image

	TileBuffer tileBuffer[2];
	xvTile *pTmpTile;

	uint8_t *dramInBuf[2];    // pointer of input double-buffer in DataRAM
	uint8_t *dramOutBuf[2];   // pointer of output double-buffer in DataRAM
	uint16_t *tempBuf;        // pointer of temporary buffer in DataRAM
	xi_array tempArray;       // temporary array used in affine kernel

	// statistics
	uint32_t intrCount;
	uint32_t kernelCycles;
	uint32_t dmaSetupCycles;
	uint32_t dmaBytes;
}ResizeMaskContext_t,*pResizeMaskContext_t;

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
typedef struct {
	xvTileManager tileMgr;
	pResizeMaskContext_t pResizeMaskContext;

	// DMA parameters
	int maxPIFBlockSize;
	int numOutstandingPIFReqs;

	// statistics
	uint32_t intrCount;
	uint32_t kernelCycles;
	uint32_t dmaSetupCycles;
	uint32_t dmaBytes;
} CadenceContext, *pCadenceContext;

typedef struct
{
	MByte *pData;
	MInt32 iWidth;//mask width
	MInt32 iHeight;//mask height
	MInt32 iLineStep;//mask buffer line stride in byte uint
	MInt32 iMaskReduce;//ratio to image;mask width  = image width / iMaskReduce;mask height  = image height / iMaskReduce;
}ResizeMask_t,*pResizeMask_t;

typedef struct
{
	ResizeMask_t srcMask;
	ResizeMask_t dstMask;
	float fwratio;				//ratio for width
	float fhratio;			 	//ratio for height
}ResizeMaskMap,*pResizeMaskMap;

typedef struct
{
	int srcWidth;
	int srcHeight;
	int dstWidth;
	int dstHeight;
}rectMap_t;

MRESULT _ResizeMask_ref(const pResizeMask_t pImgMaskSrc,pResizeMask_t pImgMaskDst);
MHandle InitializeCadenceContext(pCadenceContext cadenceCtx);
MInt32 ResizeMaskCadence(MHandle CadenceHandle, pResizeMaskMap pMask);
ErrorType DestoryResizeMaskContext(pResizeMaskContext_t ctx);

#endif /*  */
