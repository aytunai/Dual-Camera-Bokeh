/*
 * Combine.c
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#include "Combine.h"


#define UV_SEP_TILE_WIDTH 	128
#define UV_SEP_TILE_HEIGHT	128
#define UV_SEP_TILE_PITCH	128

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
#define IN_BUFF_NUM	2
typedef struct {

	xvTileManager tileMgr;
	int tileWidth;           	// tile width
	int tileHeight;           	// tile height
	int tilePitch;			  	// tile pitch
	int numTiles;             	// number of tiles in an image segment

	// DMA Hardware related parameters
	int maxPIFBlockSize;
	int numOutstandingPIFReqs;

	//Buffer:UV,blurUV,alpha,Output
	//system RAM Buffer(frame buffer)
	xvFrame *pUFrame;    		// U image
	xvFrame *pVFrame;    		// V image
	xvFrame *pUVFrame;    		// U/V image


	//DataRAM double-buffer(return pre-allocate tile array)
	xvTile* tileIn[2][IN_BUFF_NUM];
	xvTile* tileOut[2];

	//allocate tile buffer(double-buffer in DataRAM)
	uint8_t *pUV[2];
	uint8_t *pU[2];
	uint8_t *pV[2];

	//局部变量使用,记录tile的信息,都保存在Ctx结构体当中
	xi_tile srcTile;          	// source tile info
	xi_tile dstTile;          	// output tile info

	// runtime related
	xi_point srcPos[2];			//更新当前Tile的位置
	xi_point dstPos[2];
	int x_loop, y_loop;
	int x_remain, y_remain;

}UV_Combine_Context, *pUV_Combine_Context;

/*
 * Initialize context.
 */
//UV_Combine_InitializeContext(&ctx , in1 , in2 , alpha , out , width , height , pitch);
static ErrorType UV_Combine_InitializeContext(UV_Combine_Context *ctx , uint8_t *pUV , uint8_t *pU , uint8_t *pV ,
		int width , int height , int pitch)
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
	int swidth  = width  , dwidth  = width * 2;
	int sheight = height , dheight = height;
	int spitch  = pitch  , dpitch  = pitch * 2;
	int sframeSize = spitch * sheight , dframeSize = dpitch * dheight;

	ctx->srcTile.width  = UV_SEP_TILE_WIDTH;
	ctx->srcTile.height = UV_SEP_TILE_HEIGHT;
	ctx->srcTile.pitch  = UV_SEP_TILE_PITCH;
	ctx->dstTile.width  = UV_SEP_TILE_WIDTH * 2;
	ctx->dstTile.height = UV_SEP_TILE_HEIGHT;
	ctx->dstTile.pitch  = UV_SEP_TILE_PITCH * 2;

	ctx->tileWidth = UV_SEP_TILE_WIDTH , ctx->tileHeight = UV_SEP_TILE_HEIGHT;
	ctx->tilePitch = UV_SEP_TILE_PITCH;

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
	//--------------------------------------------------------------------- Frame Buffer---------------------------------------------------------------------//
	// Allocate source frame in TM, and set its data pointer to the source frame buffer.
	ctx->pUVFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pUVFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pUFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pUFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pVFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pVFrame == XVTM_ERROR) {
		return TM_ERROR;
	}

	SETUP_FRAME(ctx->pUFrame  , pU  , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);
	SETUP_FRAME(ctx->pVFrame  , pV  , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);
	SETUP_FRAME(ctx->pUVFrame , pUV , dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);


	//--------------------------------------------------------------------- Tile Buffer---------------------------------------------------------------------//
	// Allocate input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	int tileInputBuffSize = (ctx->srcTile.pitch) * (ctx->srcTile.height);
	int tileOutputBuffSize= (ctx->dstTile.pitch) * (ctx->dstTile.height);


	// Allocate Input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	// Allocate Output tile 0/1 in DataRAM0/1
	ctx->pU[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pU[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pU[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pU[1] == XVTM_ERROR) {
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
	SETUP_TILE(ctx->tileIn[0][0] , ctx->pU[0], tileInputBuffSize, ctx->pUFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1][0] , ctx->pU[1], tileInputBuffSize, ctx->pUFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);


	ctx->pV[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pV[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pV[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pV[1] == XVTM_ERROR) {
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
	SETUP_TILE(ctx->tileIn[0][1] , ctx->pV[0], tileInputBuffSize, ctx->pVFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1][1] , ctx->pV[1], tileInputBuffSize, ctx->pVFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);


	// Allocate Output tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	// Allocate Output tile 0/1 in DataRAM0/1
	ctx->pUV[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pUV[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pUV[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pUV[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileOut[0] , ctx->pUV[0], tileOutputBuffSize, ctx->pUVFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileOut[1] , ctx->pUV[1], tileOutputBuffSize, ctx->pUVFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	return NO_ERROR;
}

/*
 * Schedule DMA for an input tile.
 *
 * The input tile position and size is computed by output tile position and size using
 * the affine matrix. Then the input/output tile parameters are updated correspondingly.
 *
 * ctx - pointer of context
 * srcPos - pointer of input tile position
 * dstPos - pointer of output tile position
 * phase  - pointer of current tile buffer. A tile buffer contains one input tile and
 *           one output tile. The processing kernel runs on a tile buffer, applying
 *           affine transform on the input tile and write the result to the output tile.
 */
static inline ErrorType ScheduleInputDMA(UV_Combine_Context *ctx, xi_point *srcPos ,  xi_point *dstPos , int phase)
{
	xvTileManager *pTM = &ctx->tileMgr;

	xi_size srcSize, dstSize;

	// Compute input tile position and size using output tile position and size.
	dstSize.width 	= MAX(0, MIN(ctx->dstTile.width,  ctx->pUVFrame->frameWidth - dstPos->x));
	dstSize.height 	= MAX(0, MIN(ctx->dstTile.height, ctx->pUVFrame->frameHeight - dstPos->y));
	srcSize.width 	= MAX(0, MIN(ctx->srcTile.width,  ctx->pUFrame->frameWidth - srcPos->x));
	srcSize.height 	= MAX(0, MIN(ctx->srcTile.height, ctx->pUFrame->frameHeight - srcPos->y));

	// Update output tile parameters
	XV_TILE_UPDATE_DIMENSIONS(ctx->tileOut[phase], dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);
	int tw = ((srcPos->x + ctx->srcTile.width) >= ctx->pUFrame->frameWidth) ? (srcSize.width):(ctx->srcTile.width);
	int th = ((srcPos->y + ctx->srcTile.height) >= ctx->pUFrame->frameHeight) ? (srcSize.height):(ctx->srcTile.height);
	// Update input tile parameters and issue the DMA request
	int i;
	for(i = 0 ; i < IN_BUFF_NUM ; i++){
		XV_TILE_UPDATE_DIMENSIONS(ctx->tileIn[phase][i], srcPos->x, srcPos->y, tw,th, ctx->srcTile.pitch);
	}
	// Update the next tile position
	dstPos->x += dstSize.width;
	srcPos->x += srcSize.width;
	if (dstPos->x >= ctx->pUVFrame->frameWidth) {
		dstPos->y += dstSize.height;
		dstPos->x = 0;
	}
	if(srcPos->x >= ctx->pUFrame->frameWidth){
		srcPos->y += srcSize.height;
		srcPos->x = 0;
	}

	//tranfer data from external memory bank to dram
	for(i = 0 ; i < IN_BUFF_NUM ; i++){
		int32_t retVal = xvReqTileTransferInFast(pTM, ctx->tileIn[phase][i]);
	}
	return NO_ERROR;
}

//内部保证数据是aligned的(DMA传输和计算都能达到较好的效果)
void UV_Combine_Cadence(UV_Combine_Context *ctx , int phase)
{
	xvTile *pTileU  = ctx->tileIn[phase][0];
	xvTile *pTileV  = ctx->tileIn[phase][1];

	xvTile *pTileUV = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileUV);
	int dstride = XI_TILE_GET_PITCH(pTileU);
	int width   = XI_TILE_GET_WIDTH(pTileU);
	int height  = XI_TILE_GET_HEIGHT(pTileU);

	//UV定义的时候为uint16_t,因此这里一次性读取两个数据
	xb_vec2Nx8U* pUV = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileUV);
	xb_vec2Nx8U* pU  = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileU);
	xb_vec2Nx8U* pV  = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileV);

	xb_vec2Nx8U* __restrict rUV = pUV;
	xb_vec2Nx8U* __restrict rU  = pU;
	xb_vec2Nx8U* __restrict rV  = pV;

	int i , j = 0;
	int num = dstride * height / (2 * XCHAL_IVPN_SIMD_WIDTH);//(dstride * height) >> 6;

	for(i = 0 ; i < num ; i++){
		xb_vec2Nx8U vU = *rU++;
		xb_vec2Nx8U vV = *rV++;

		xb_vec2Nx8U vUV_Low  = IVP_SEL2NX8UI(vV, vU, IVP_SELI_8B_INTERLEAVE_1_LO);
		xb_vec2Nx8U vUV_High = IVP_SEL2NX8UI(vV, vU, IVP_SELI_8B_INTERLEAVE_1_HI);

		*rUV++ = vUV_Low;
		*rUV++ = vUV_High;
	}
}

void UV_Combine_C(UV_Combine_Context *ctx , int phase)
{
	xvTile *pTileU  = ctx->tileIn[phase][0];
	xvTile *pTileV  = ctx->tileIn[phase][1];

	xvTile *pTileUV = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileU);
	int dstride = XI_TILE_GET_PITCH(pTileUV);
	int width   = XI_TILE_GET_WIDTH(pTileU);
	int height  = XI_TILE_GET_HEIGHT(pTileU);

	uint8_t* pUV = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileUV);
	uint8_t*  pU = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileU);
	uint8_t*  pV = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileV);

	int i , j;
	for(j = 0 ; j < height ; j ++ , pU += sstride , pV += sstride, pUV += dstride ){

		if(j == (height / 2)){
			j = (height / 2);
		}
		uint8_t* pTmp = (uint8_t *)pUV;

		for(i = 0 ; i < width ; i++){
			*pTmp++ = pU[i];
			*pTmp++ = pV[i];
		}
	}



}

static ErrorType UV_Combine_Process(UV_Combine_Context *ctx)
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
	int numTiles = ((ctx->pUFrame->frameHeight + ctx->tileHeight - 1) / ctx->tileHeight)
			* ((ctx->pUFrame->frameWidth + ctx->tileWidth - 1) / ctx->tileWidth);

	// Tile processing loop
	int i;
	for (i = 0; i < numTiles; i++) {
		// Wait for the last input tile transfer to be finished
		WAIT_FOR_TILE_FAST(pTM, ctx->tileIn[bufIndex][IN_BUFF_NUM - 1]);

		// Process the Buffer
#ifdef COMBINE_CADENCE_OPT
		UV_Combine_Cadence(ctx , bufIndex);
#else
		UV_Combine_C(ctx , bufIndex);
#endif

		// Schedule output DMA
		int32_t error1 = xvReqTileTransferOutFast(pTM, ctx->tileOut[bufIndex]);
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
	return NO_ERROR;
}


/*
 * Release context.
 */
static ErrorType UV_Combine_ReleaseContext(UV_Combine_Context *ctx)
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


void UV_Combine_U8(uint8_t *pUV , uint8_t *pU , uint8_t *pV , int width , int height , int stride)
{
	UV_Combine_Context ctx;

	int errType = UV_Combine_InitializeContext( &ctx , pUV , pU , pV , width , height , stride);
	if(errType == TM_ERROR){
		xa_printf("UV_Combine_InitializeContext failed\n");
		return ;
	}

	UV_Combine_Process(&ctx);

	UV_Combine_ReleaseContext(&ctx);



}

