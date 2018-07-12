/*
 *
 * Separate.c
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#include "Separate.h"


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
#define OUT_BUFF_NUM	2
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
	xvFrame *pUVFrame;    		// U/V image
	xvFrame *pUFrame;    		// Blur U/V image
	xvFrame *pVFrame;    		// U/V image

	//DataRAM double-buffer(return pre-allocate tile array)
	xvTile* tileIn[2];
	xvTile* tileOut[2][OUT_BUFF_NUM];

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

}Separate_UV_Context, *pSeparate_UV_Context;

/*
 * Initialize context.
 */
//Separate_UV_InitializeContext(&ctx , in1 , in2 , alpha , out , width , height , pitch);
static ErrorType Separate_UV_InitializeContext(Separate_UV_Context *ctx , uint8_t *pUV , uint8_t *pU , uint8_t *pV ,
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
	int swidth  = width * 2 , dwidth  = width;
	int sheight = height , dheight = height;
	int spitch  = pitch * 2 , dpitch  = pitch;
	int sframeSize = spitch * sheight , dframeSize = dpitch * dheight;

	ctx->srcTile.width  = UV_SEP_TILE_WIDTH * 2;
	ctx->srcTile.height = UV_SEP_TILE_HEIGHT;
	ctx->srcTile.pitch  = UV_SEP_TILE_PITCH * 2;
	ctx->dstTile.width  = UV_SEP_TILE_WIDTH;
	ctx->dstTile.height = UV_SEP_TILE_HEIGHT;
	ctx->dstTile.pitch  = UV_SEP_TILE_PITCH;

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
	SETUP_FRAME(ctx->pUVFrame , pUV , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);
	SETUP_FRAME(ctx->pUFrame  , pU  , dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);
	SETUP_FRAME(ctx->pVFrame  , pV  , dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);


	//--------------------------------------------------------------------- Tile Buffer---------------------------------------------------------------------//
	// Allocate input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	int tileInputBuffSize = (ctx->srcTile.pitch) * (ctx->srcTile.height);
	int tileOutputBuffSize= (ctx->dstTile.pitch) * (ctx->dstTile.height);

	// Allocate UV tile 0/1 in DataRAM0/1
	ctx->pUV[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pUV[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pUV[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pUV[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileIn[1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileIn[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileIn[0] , ctx->pUV[0], tileInputBuffSize, ctx->pUVFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1] , ctx->pUV[1], tileInputBuffSize, ctx->pUVFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	// Allocate Output tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	// Allocate Output tile 0/1 in DataRAM0/1
	ctx->pU[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pU[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pU[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pU[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[0][0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[0][0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[1][0] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[1][0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileOut[0][0] , ctx->pU[0], tileOutputBuffSize, ctx->pUFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileOut[1][0] , ctx->pU[1], tileOutputBuffSize, ctx->pUFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);


	ctx->pV[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pV[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pV[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pV[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[0][1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[0][1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileOut[1][1] = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileOut[1][1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileOut[0][1] , ctx->pV[0], tileOutputBuffSize, ctx->pVFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileOut[1][1] , ctx->pV[1], tileOutputBuffSize, ctx->pVFrame, ctx->dstTile.width,
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
static inline ErrorType ScheduleInputDMA(Separate_UV_Context *ctx, xi_point *srcPos ,  xi_point *dstPos , int phase)
{
	xvTileManager *pTM = &ctx->tileMgr;

	xi_size srcSize, dstSize;

	// Compute input tile position and size using output tile position and size.
	dstSize.width 	= MAX(0, MIN(ctx->dstTile.width,  ctx->pUFrame->frameWidth - dstPos->x));
	dstSize.height 	= MAX(0, MIN(ctx->dstTile.height, ctx->pUFrame->frameHeight - dstPos->y));
	srcSize.width 	= MAX(0, MIN(ctx->srcTile.width,  ctx->pUVFrame->frameWidth - srcPos->x));
	srcSize.height 	= MAX(0, MIN(ctx->srcTile.height, ctx->pUVFrame->frameHeight - srcPos->y));

	// Update output tile parameters
	int i;
	for(i = 0 ; i < OUT_BUFF_NUM ; i++){
		//XV_TILE_UPDATE_DIMENSIONS((ctx->tileOut[phase][i]), dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);
		XV_TILE_UPDATE_DIMENSIONS( ctx->tileOut[phase][i], dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);
	}
	int tw = ((srcPos->x + ctx->srcTile.width) >= ctx->pUVFrame->frameWidth) ? (srcSize.width):(ctx->srcTile.width);
	int th = ((srcPos->y + ctx->srcTile.height) >= ctx->pUVFrame->frameHeight) ? (srcSize.height):(ctx->srcTile.height);
	// Update input tile parameters and issue the DMA request
	XV_TILE_UPDATE_DIMENSIONS(ctx->tileIn[phase], srcPos->x, srcPos->y, tw,th, ctx->srcTile.pitch);

	// Update the next tile position
	dstPos->x += dstSize.width;
	srcPos->x += srcSize.width;
	if (dstPos->x >= ctx->pUFrame->frameWidth) {
		dstPos->y += dstSize.height;
		dstPos->x = 0;
	}
	if(srcPos->x >= ctx->pUVFrame->frameWidth){
		srcPos->y += srcSize.height;
		srcPos->x = 0;
	}

	//tranfer data from external memory bank to dram
	int32_t retVal = xvReqTileTransferInFast(pTM, ctx->tileIn[phase]);

	return NO_ERROR;
}

//内部保证数据是aligned的(DMA传输和计算都能达到较好的效果)
void Separate_UV_Cadence(Separate_UV_Context *ctx , int phase)
{
	xvTile *pTileUV = ctx->tileIn[phase];
	xvTile *pTileU  = ctx->tileOut[phase][0];
	xvTile *pTileV  = ctx->tileOut[phase][1];

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
		xb_vec2Nx8U vUV_Low  = *rUV++;
		xb_vec2Nx8U vUV_High = *rUV++;

		xb_vec2Nx8U vU = IVP_SEL2NX8UI(vUV_High, vUV_Low, IVP_SELI_8B_EXTRACT_1_OF_2_OFF_0);
		xb_vec2Nx8U vV = IVP_SEL2NX8UI(vUV_High, vUV_Low, IVP_SELI_8B_EXTRACT_1_OF_2_OFF_1);

		*rU++ = vU;
		*rV++ = vV;
	}
}

void Separate_UV_C(Separate_UV_Context *ctx , int phase)
{
	xvTile *pTileUV = ctx->tileIn[phase];
	xvTile *pTileU  = ctx->tileOut[phase][0];
	xvTile *pTileV  = ctx->tileOut[phase][1];

	int sstride = XI_TILE_GET_PITCH(pTileUV);
	int dstride = XI_TILE_GET_PITCH(pTileU);
	int width   = XI_TILE_GET_WIDTH(pTileU);
	int height  = XI_TILE_GET_HEIGHT(pTileU);

	uint8_t* pUV = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileUV);
	uint8_t*  pU = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileU);
	uint8_t*  pV = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileV);

	int i , j;
	for(j = 0 ; j < height ; j ++ , pUV += sstride , pU += dstride , pV += dstride){
		uint8_t* pTmp = pUV;
		for(i = 0 ; i < width ; i++){

			pU[i] = pTmp[2 * i];
			pV[i] = pTmp[2 * i + 1];

			int tu = pU[i] , tv = pV[i];
			int idx = i;

		}
	}

}

static ErrorType Separate_UV_Process(Separate_UV_Context *ctx)
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
	int i , k;
	for (i = 0; i < numTiles; i++) {
		// Wait for the last input tile transfer to be finished
		WAIT_FOR_TILE_FAST(pTM, ctx->tileIn[bufIndex]);

		// Process the Buffer
#ifdef SEPARATE_OPT_CADENCE
		Separate_UV_Cadence(ctx , bufIndex);
#else
		Separate_UV_C(ctx , bufIndex);
#endif

		// Schedule output DMA
		for(k = 0 ; k < OUT_BUFF_NUM ; k++){
			int32_t error1 = xvReqTileTransferOutFast(pTM, ctx->tileOut[bufIndex][k]);
			if (error1 != XVTM_SUCCESS) {
#pragma frequency_hint never
				error = TM_ERROR;
				break;
			}
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
	WAIT_FOR_TILE_FAST(pTM, ctx->tileOut[bufIndex ^ 1][OUT_BUFF_NUM - 1]);

errorHandler:
	return NO_ERROR;
}


/*
 * Release context.
 */
static ErrorType Separate_UV_ReleaseContext(Separate_UV_Context *ctx)
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

void UV_Separate_U8(uint8_t *pUV , uint8_t *pU , uint8_t *pV , int width , int height , int stride)
{
	Separate_UV_Context ctx;

	Separate_UV_InitializeContext( &ctx , pUV , pU , pV , width , height , stride);

	Separate_UV_Process(&ctx);

	Separate_UV_ReleaseContext(&ctx);
}






