/*
 * AlphaBlend.c
 *
 *  Created on: 2018-7-9
 *      Author: AiTong
 */

#include "stdio.h"
#include "AlphaBlend.h"

/*
 * Function Description:
 * 			Bokeh[i,j] = (Y[i,j] * Alpha[i,j] + BlurY[i,j] * (255 - Alpha[i,j])) / 256;
 */

//自定义的tile的大小,pitch需要aligned 64B
#define ALPHA_BLEND_TILE_WIDTH			128
#define ALPHA_BLEND_TILE_HEIGHT 		64
#define ALPHA_BLEND_TILE_PITCH		 	128


/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
#define IN_BUFF_NUM		3
typedef struct {

	xvTileManager tileMgr;		// Memory Pools
	int tileWidth;           	// tile width
	int tileHeight;           	// tile height
	int tilePitch;			  	// tile pitch
	int numTiles;             	// number of tiles in an image segment

	// DMA Hardware related parameters
	int maxPIFBlockSize;
	int numOutstandingPIFReqs;

	//system RAM Buffer(frame buffer)
	xvFrame *pSrcFrame;    		// source Y image
	xvFrame *pBlurFrame;    	// blur Y image
	xvFrame *pDepthFrame;    	// depth image
	xvFrame *pOutFrame;     	// output image

	//DataRAM double-buffer(return pre-allocate tile array)
	xvTile* tileIn[2][IN_BUFF_NUM];
	xvTile* tileOut[2];

	//DataRAM double-buffer(tile buffer)(ping-pong buffer)
	uint8_t *pdramIn[2];		// pointer of input  double-buffer in DataRAM(return allocate origin pointer)
	uint8_t *pdramInBlur[2];	// pointer of input  double-buffer in DataRAM(return allocate origin pointer)
	uint8_t *pdramDepth[2];		// pointer of input  double-buffer in DataRAM(return allocate origin pointer)
	uint8_t *pdramOut[2];		// pointer of output double-buffer in DataRAM(return allocate origin pointer)

	//局部变量使用,记录tile的信息,都保存在Ctx结构体当中
	xi_tile srcTile;          	// source tile info
	xi_tile dstTile;          	// output tile info

	// runtime related
	xi_point srcPos[2];			//更新当前Tile的位置
	xi_point dstPos[2];

}AlphaBlend_Context, *pAlphaBlend_Context;

/*
 * Initialize context.
 */
static ErrorType AlphaBlend_InitializeContext(AlphaBlend_Context *ctx , uint8_t *pSrc , uint8_t *pBlur , uint8_t *pDepth ,
		int swidth , int sheight , int spitch , uint8_t *pOut , int dwidth , int dheight , int dpitch)
{
	// Set DMA parameters. Changing DMA parameters and tile size will affect the
	// effective DMA throughput.

	// Maximum PIF block size
	//   ctx->maxPIFBlockSize = 0,  max PIF block size is 2 (32 bytes)
	//   ctx->maxPIFBlockSize = 1,  max PIF block size is 4 (64 bytes)
	//   ctx->maxPIFBlockSize = 2,  max PIF block size is 8 (128 bytes)
	//   ctx->maxPIFBlockSize = 3,  max PIF block size is 16 (256 bytes)
	ctx->maxPIFBlockSize = 2;
	// Number of outstanding PIF requests allowed
	ctx->numOutstandingPIFReqs = 32;

	//Config the source and destination pitch aligned with 64Bytes
	int32_t sframeSize = spitch * sheight;
	int32_t dframeSize = dpitch * dheight;

	ctx->srcTile.width  = ALPHA_BLEND_TILE_WIDTH;
	ctx->srcTile.height = ALPHA_BLEND_TILE_HEIGHT;
	ctx->srcTile.pitch  = ALPHA_BLEND_TILE_PITCH;
	ctx->dstTile.width  = ALPHA_BLEND_TILE_WIDTH;
	ctx->dstTile.height = ALPHA_BLEND_TILE_HEIGHT;
	ctx->dstTile.pitch  = ALPHA_BLEND_TILE_PITCH;

	ctx->tileWidth = ALPHA_BLEND_TILE_WIDTH , ctx->tileHeight = ALPHA_BLEND_TILE_HEIGHT;
	ctx->tilePitch = ALPHA_BLEND_TILE_PITCH;

	// Initialize TileManager
	xvTileManager *pTM = &ctx->tileMgr;
	uint32_t retVal = xvInitTileManager(pTM, (void *)p_idma_2d_desc);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	// Initialize the DMA
	retVal = xvInitIdma(pTM, (idma_buffer_t *)p_idma_2d_desc, DMA_DESCR_CNT, ctx->maxPIFBlockSize,
			ctx->numOutstandingPIFReqs, errCallbackFunc, intrCallbackFunc, (void *)ctx);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	// Initializing memory allocator. Here we have 2 physical DataRAMs, so we allocate 2
	// memory pools, one for each DataRAM.
	void *buffPool[2];
	buffPool[0] = pDramPool0;
	buffPool[1] = pDramPool1;
	int32_t buffSize[2];
	buffSize[0] = DRAM_POOL_SIZE;
	buffSize[1] = DRAM_POOL_SIZE;
	retVal = xvInitMemAllocator(pTM, 2, buffPool, buffSize);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	// Allocate source frame in TM, and set its data pointer to the source frame buffer.
	ctx->pSrcFrame   = xvAllocateFrame(pTM);
	ctx->pBlurFrame  = xvAllocateFrame(pTM);
	ctx->pDepthFrame = xvAllocateFrame(pTM);
	ctx->pOutFrame   = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pSrcFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pBlurFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pDepthFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pOutFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_FRAME(ctx->pSrcFrame  , pSrc   , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);
	SETUP_FRAME(ctx->pBlurFrame , pBlur  , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);
	SETUP_FRAME(ctx->pDepthFrame, pDepth , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);
	SETUP_FRAME(ctx->pOutFrame  , pOut   , dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);

	// Allocate input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	int tileInputBuffSize  = (ctx->srcTile.pitch) * (ctx->srcTile.height);
	int tileOutputBuffSize = (ctx->dstTile.pitch) * (ctx->dstTile.height);
	/******************************************************************************************************************************/
	/*													SrcImg																	  */
	/******************************************************************************************************************************/
	// Allocate SrcImg tile 0/1 in DataRAM 0/1
	ctx->pdramIn[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramIn[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramIn[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramIn[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[0][0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[0][0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[1][0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[1][0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileIn[0][0] , ctx->pdramIn[0], tileInputBuffSize, ctx->pSrcFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1][0] , ctx->pdramIn[1], tileInputBuffSize, ctx->pSrcFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	/******************************************************************************************************************************/
	/*													pBlurFrame																  */
	/******************************************************************************************************************************/
	// Allocate SrcImg tile 0/1 in DataRAM 0/1
	ctx->pdramInBlur[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramInBlur[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramInBlur[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramInBlur[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[0][1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[0][1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[1][1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[1][1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileIn[0][1] , ctx->pdramInBlur[0], tileInputBuffSize, ctx->pBlurFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1][1] , ctx->pdramInBlur[1], tileInputBuffSize, ctx->pBlurFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	/******************************************************************************************************************************/
	/*													pDepthFrame																  */
	/******************************************************************************************************************************/
	// Allocate SrcImg tile 0/1 in DataRAM 0/1
	ctx->pdramDepth[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramDepth[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramDepth[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramDepth[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[0][2] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[0][2] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[1][2] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[1][2] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileIn[0][2] , ctx->pdramDepth[0], tileInputBuffSize, ctx->pDepthFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1][2] , ctx->pdramDepth[1], tileInputBuffSize, ctx->pDepthFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	/******************************************************************************************************************************/
	/*													OutputImg																	  */
	/******************************************************************************************************************************/
	// Allocate Output tile 0/1 in DataRAM 0/1
	ctx->pdramOut[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramOut[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramOut[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramOut[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[0] = xvAllocateTile(pTM);
	ctx->tileOut[1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->tileOut[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileOut[0] , ctx->pdramOut[0], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileOut[1] , ctx->pdramOut[1], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	return NO_ERROR;
}

/*
 * Schedule DMA for an input tile.
 *
 * The input tile position and size is computed by output tile position and size using
 * the affine matrix. Then the input/output tile parameters are updated correspondingly.
 *
 * ctx    - pointer of context
 * srcPos - pointer of input tile position
 * dstPos - pointer of output tile position
 * phase  - index of current tile buffer.
 */
static inline ErrorType ScheduleInputDMA(AlphaBlend_Context *ctx, xi_point *srcPos ,  xi_point *dstPos, int phase)
{
	xvTileManager *pTM = &ctx->tileMgr;

	xi_size srcSize, dstSize;

	// Compute input tile position and size using output tile position and size.
	dstSize.width 	= MAX(0, MIN(ctx->dstTile.width,  ctx->pOutFrame->frameWidth  - dstPos->x));
	dstSize.height 	= MAX(0, MIN(ctx->dstTile.height, ctx->pOutFrame->frameHeight - dstPos->y));
	srcSize.width 	= MAX(0, MIN(ctx->srcTile.width,  ctx->pSrcFrame->frameWidth  - srcPos->x));
	srcSize.height 	= MAX(0, MIN(ctx->srcTile.height, ctx->pSrcFrame->frameHeight - srcPos->y));

	// Update output tile parameters
	XV_TILE_UPDATE_DIMENSIONS(ctx->tileOut[phase], dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);

	int tw = ((srcPos->x + ctx->srcTile.width)  >= ctx->pSrcFrame->frameWidth)  ? (srcSize.width):(ctx->srcTile.width);
	int th = ((srcPos->y + ctx->srcTile.height) >= ctx->pSrcFrame->frameHeight) ? (srcSize.height):(ctx->srcTile.height);
	// Update input tile parameters and issue the DMA request
	int i;
	for(i = 0 ; i < IN_BUFF_NUM ; i++){
		XV_TILE_UPDATE_DIMENSIONS(ctx->tileIn[phase][i], srcPos->x, srcPos->y, tw,th, ctx->srcTile.pitch);
	}

	// Update the next tile position
	dstPos->x += dstSize.width;
	srcPos->x += srcSize.width;
	if (dstPos->x >= ctx->pOutFrame->frameWidth) {
		dstPos->y += dstSize.height;
		dstPos->x = 0;
	}
	if(srcPos->x >= ctx->pSrcFrame->frameWidth){
		srcPos->y += srcSize.height;
		srcPos->x = 0;
	}

	//tranfer data from external memory bank to dram
	int32_t retVal;
	for(i = 0 ; i < IN_BUFF_NUM ; i++){
		retVal = xvReqTileTransferInFast(pTM, ctx->tileIn[phase][i]);
	}
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	return NO_ERROR;
}

//Y-component AlphaBlend,需要保证tile中的stride为align的buffer
static ErrorType AlphaBlend_U8_Cadence_V1(AlphaBlend_Context *ctx , int phase)
{
	xvTile *pTileSrc   = ctx->tileIn[phase][0];
	xvTile *pTileBlur  = ctx->tileIn[phase][1];
	xvTile *pTileDepth = ctx->tileIn[phase][2];
	xvTile *pTileOut   = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileSrc);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileOut);
	int height  = XI_TILE_GET_HEIGHT(pTileOut);

	xb_vec2Nx8U* psrc   = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileSrc);
	xb_vec2Nx8U* pblur  = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileBlur);
	xb_vec2Nx8U* pdepth = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileDepth);
	xb_vec2Nx8U* pdst   = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileOut);

	int i , j = 0;
	valign a_load , a_store = IVP_ZALIGN();
	for(; j < width - XCHAL_IVPN_SIMD_WIDTH ; j += 2 * XCHAL_IVPN_SIMD_WIDTH , psrc ++ , pblur++ , pdepth++ , pdst++){

		xb_vec2Nx8U* __restrict rInA   = psrc;
		xb_vec2Nx8U* __restrict rInB   = pblur;
		xb_vec2Nx8U* __restrict rAlpha = pdepth;
		xb_vec2Nx8U* __restrict rdst   = pdst;

		int offs = sstride - (width - j - 2 * XCHAL_IVPN_SIMD_WIDTH);
		int offd = dstride - (width - j - 2 * XCHAL_IVPN_SIMD_WIDTH);

		for(i = 0 ; i < height ; i++){

			xb_vec2Nx8U vecInA   = *rInA;
			rInA = OFFSET_PTR_2NX8U(rInA , 1 , sstride , 0);
			xb_vec2Nx8U vecInB   = *rInB;
			rInB = OFFSET_PTR_2NX8U(rInB , 1 , sstride , 0);
			xb_vec2Nx8U vecAlpha = *rAlpha;
			rAlpha = OFFSET_PTR_2NX8U(rAlpha , 1 , sstride , 0);

			xb_vec2Nx24 w = IVP_MULUU2NX8U(vecInA, vecAlpha);
			IVP_MULUSA2NX8U(w, vecInB, IVP_NOT2NX8U(vecAlpha) );
			xb_vec2Nx8U vecOut = IVP_PACKVR2NX24(w, 8);
			*rdst = vecOut;
			rdst = OFFSET_PTR_2NX8U(rdst , 1 , dstride , 0);
		}
	}

	return NO_ERROR;
}

//AlphaBlend,需要保证tile中的stride都需要align with 64B,Tile在搬移的时候会自动截取有效的区域到System RAM中
static ErrorType AlphaBlend_U8_Cadence(AlphaBlend_Context *ctx , int phase)
{
	xvTile *pTileSrc   = ctx->tileIn[phase][0];
	xvTile *pTileBlur  = ctx->tileIn[phase][1];
	xvTile *pTileDepth = ctx->tileIn[phase][2];
	xvTile *pTileOut   = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileSrc);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileOut);
	int height  = XI_TILE_GET_HEIGHT(pTileOut);

	xb_vec2Nx8U* psrc   = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileSrc);
	xb_vec2Nx8U* pblur  = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileBlur);
	xb_vec2Nx8U* pdepth = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileDepth);
	xb_vec2Nx8U* pdst   = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileOut);

	xb_vec2Nx8U* __restrict rInA   = psrc;
	xb_vec2Nx8U* __restrict rInB   = pblur;
	xb_vec2Nx8U* __restrict rAlpha = pdepth;
	xb_vec2Nx8U* __restrict rdst   = pdst;

	int i , numTile = (dstride * height) >> 6;
	for(i = 0 ; i < numTile ; i++){
		xb_vec2Nx8U vecInA   = *rInA++;
		xb_vec2Nx8U vecInB   = *rInB++;
		xb_vec2Nx8U vecAlpha = *rAlpha++;

		xb_vec2Nx24 w = IVP_MULUU2NX8U(vecInA, vecAlpha);
		xb_vec2Nx8U vecNot = IVP_NOT2NX8U(vecAlpha);
		IVP_MULUUA2NX8U(w, vecInB,  vecNot);
		xb_vec2Nx8U vecOut = IVP_PACKVRU2NX24(w, 8);
		*rdst++ = vecOut;
	}

	return NO_ERROR;
}

//Y-component AlphaBlend , size = 3x3,C版本
static ErrorType AlphaBlend_U8_C(TileBuffer *buffer , int phase)
{
	return NO_ERROR;
}

/*
 * Process context.
 */
static ErrorType AlphaBlend_Process(AlphaBlend_Context *ctx)
{
	xvTileManager *pTM = &ctx->tileMgr; // get pointer of TileManager
	int32_t bufIndex = 0; // this is the double-buffer index
	xi_point srcPos = {0,0}; // this is used to track input tile position
	xi_point dstPos = {0,0}; // this is used to track output tile position

	ErrorType error;

	// Schedule 2 input DMA transfers at the beginning to hide the initial DMA transfer latency
	error = ScheduleInputDMA(ctx, &srcPos , &dstPos, 0);
	error = ScheduleInputDMA(ctx, &srcPos , &dstPos, 1);

	if (error != NO_ERROR) {
#pragma frequency_hint never
		goto errorHandler;
	}

	// Calculate how many tiles we need to process
	int numTiles = ((ctx->pOutFrame->frameHeight + ctx->tileHeight - 1) / ctx->tileHeight)
			* ((ctx->pOutFrame->frameWidth + ctx->tileWidth - 1) / ctx->tileWidth);

	// Tile processing loop
	int i;
	for (i = 0; i < numTiles; i++) {
		// Wait for the input tile transfer to be finished
		WAIT_FOR_TILE_FAST(pTM, ctx->tileIn[bufIndex][IN_BUFF_NUM - 1]);

		// Process the Buffer
#ifdef CADENCE_OPT_ALPHA_BLEND
		AlphaBlend_U8_Cadence(ctx , bufIndex);
#else
		AlphaBlend_U8_C(pTileBuffer , bufIndex);

#endif

		// Schedule output DMA
		int32_t error1 = xvReqTileTransferOut(pTM, ctx->tileOut[bufIndex] , 0);
		if (error1 != XVTM_SUCCESS) {
#pragma frequency_hint never
			error = TM_ERROR;
			break;
		}

		// Schedule next input DMA
		error = ScheduleInputDMA(ctx, &srcPos , &dstPos, bufIndex);
		if (error != NO_ERROR) {
#pragma frequency_hint never
			break;
		}

		// Toggle double-buffer index
		bufIndex ^= 0x1;
	}
	//后面还需要再拷贝回来一份数据
	// Wait for the last output tile transfer to be finished
	WAIT_FOR_TILE_FAST(pTM, ctx->tileOut[bufIndex ^ 1]);

errorHandler:
	xa_printf("Done\n");
}


/*
 * Release context.
 */
static ErrorType AlphaBlend_ReleaseContext(AlphaBlend_Context *ctx)
{
	xvTileManager *pTM = &ctx->tileMgr;

	// Free frames
	uint32_t retVal = xvFreeAllFrames(pTM);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	// Free tiles
	retVal = xvFreeAllTiles(pTM);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	retVal = xvResetTileManager(pTM);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}
	return NO_ERROR;
}


//AlphaBlend
void AlphaBlend_U8(uint8_t *pSrc , uint8_t *pBlur , uint8_t *pDepth , uint8_t *pOut , int width , int height , int pitch)
{

	AlphaBlend_Context ctx;

	AlphaBlend_InitializeContext(&ctx , pSrc , pBlur , pDepth , width , height , pitch , pOut , width , height , pitch);


	AlphaBlend_Process(&ctx);

	AlphaBlend_ReleaseContext(&ctx);

}









