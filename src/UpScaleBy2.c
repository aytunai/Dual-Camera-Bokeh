/*
 * DownScaleBy2.c
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#include "UpScaleBy2.h"


#define UP_SCALE_BY2_TILE_WIDTH 	256
#define UP_SCALE_BY2_TILE_HEIGHT	64
#define UP_SCALE_BY2_TILE_PITCH		256

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
typedef struct {

	xvTileManager tileMgr;
	int tileWidth;           	// tile width
	int tileHeight;           	// tile height
	int tilePitch;			  	// tile pitch
	int numTiles;             	// number of tiles in an image segment

	// DMA Hardware related parameters
	int maxPIFBlockSize;
	int numOutstandingPIFReqs;

	//system RAM Buffer(frame buffer)
	xvFrame *pInFrame;    		// Input image
	xvFrame *pOutFrame;    		// Output image

	//DataRAM double-buffer(return pre-allocate tile array)
	xvTile* tileIn[2];
	xvTile* tileOut[2];

	//allocate tile buffer(double-buffer in DataRAM)
	uint8_t *pIn[2];
	uint8_t *pOut[2];

	//局部变量使用,记录tile的信息,都保存在Ctx结构体当中
	xi_tile srcTile;          	// source tile info
	xi_tile dstTile;          	// output tile info

	// runtime related
	xi_point srcPos[2];			//更新当前Tile的位置
	xi_point dstPos[2];

}upScaleBy2_Context, *pupScaleBy2_Context;



/*
 * Initialize context.
 */
//upScaleBy2_InitializeContext(&ctx , in1 , in2 , alpha , out , width , height , pitch);
static ErrorType upScaleBy2_InitializeContext(upScaleBy2_Context *ctx , uint8_t *pIn , uint8_t *pOut ,
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
	int swidth  = width  , dwidth  = width  * 2;
	int sheight = height , dheight = height * 2;
	int spitch  = pitch  , dpitch  = pitch  * 2;
	int sframeSize = spitch * sheight ;
	int dframeSize = dpitch * dheight;

	ctx->srcTile.width  = UP_SCALE_BY2_TILE_WIDTH;
	ctx->srcTile.height = UP_SCALE_BY2_TILE_HEIGHT;
	ctx->srcTile.pitch  = UP_SCALE_BY2_TILE_PITCH;
	ctx->dstTile.width  = UP_SCALE_BY2_TILE_WIDTH  * 2;
	ctx->dstTile.height = UP_SCALE_BY2_TILE_HEIGHT * 2;
	ctx->dstTile.pitch  = UP_SCALE_BY2_TILE_PITCH  * 2;

	ctx->tileWidth = UP_SCALE_BY2_TILE_WIDTH , ctx->tileHeight = UP_SCALE_BY2_TILE_HEIGHT;
	ctx->tilePitch = UP_SCALE_BY2_TILE_PITCH;

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
	ctx->pInFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pInFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pOutFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pOutFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_FRAME(ctx->pInFrame  , pIn  , sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);
	SETUP_FRAME(ctx->pOutFrame , pOut , dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_ZERO_PADDING, 0);


	//--------------------------------------------------------------------- Tile Buffer---------------------------------------------------------------------//
	// Allocate input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	int tileInputBuffSize = (ctx->srcTile.pitch) * (ctx->srcTile.height);
	int tileOutputBuffSize= (ctx->dstTile.pitch) * (ctx->dstTile.height);

	// Allocate UV tile 0/1 in DataRAM0/1
	ctx->pIn[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pIn[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pIn[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pIn[1] == XVTM_ERROR) {
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
	SETUP_TILE(ctx->tileIn[0] , ctx->pIn[0], tileInputBuffSize, ctx->pInFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileIn[1] , ctx->pIn[1], tileInputBuffSize, ctx->pInFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);

	// Allocate Output tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	// Allocate Output tile 0/1 in DataRAM0/1
	ctx->pOut[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	if ((int32_t) ctx->pOut[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->pOut[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pOut[1] == XVTM_ERROR) {
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
	SETUP_TILE(ctx->tileOut[0] , ctx->pOut[0], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileOut[1] , ctx->pOut[1], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
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
static inline ErrorType ScheduleInputDMA(upScaleBy2_Context *ctx, xi_point *srcPos ,  xi_point *dstPos , int phase)
{
	xvTileManager *pTM = &ctx->tileMgr;

	xi_size srcSize, dstSize;

	// Compute input tile position and size using output tile position and size.
	dstSize.width 	= MAX(0, MIN(ctx->dstTile.width,  ctx->pOutFrame->frameWidth - dstPos->x));
	dstSize.height 	= MAX(0, MIN(ctx->dstTile.height, ctx->pOutFrame->frameHeight - dstPos->y));
	srcSize.width 	= MAX(0, MIN(ctx->srcTile.width,  ctx->pInFrame->frameWidth - srcPos->x));
	srcSize.height 	= MAX(0, MIN(ctx->srcTile.height, ctx->pInFrame->frameHeight - srcPos->y));

	// Update output tile parameters
	XV_TILE_UPDATE_DIMENSIONS( ctx->tileOut[phase], dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);
	int tw = ((srcPos->x + ctx->srcTile.width) >= ctx->pInFrame->frameWidth) ? (srcSize.width):(ctx->srcTile.width);
	int th = ((srcPos->y + ctx->srcTile.height) >= ctx->pInFrame->frameHeight) ? (srcSize.height):(ctx->srcTile.height);
	// Update input tile parameters and issue the DMA request
	XV_TILE_UPDATE_DIMENSIONS(ctx->tileIn[phase], srcPos->x, srcPos->y, tw,th, ctx->srcTile.pitch);

	// Update the next tile position
	dstPos->x += dstSize.width;
	srcPos->x += srcSize.width;
	if (dstPos->x >= ctx->pOutFrame->frameWidth) {
		dstPos->y += dstSize.height;
		dstPos->x = 0;
	}
	if(srcPos->x >= ctx->pInFrame->frameWidth){
		srcPos->y += srcSize.height;
		srcPos->x = 0;
	}

	//tranfer data from external memory bank to dram
	int32_t retVal = xvReqTileTransferInFast(pTM, ctx->tileIn[phase]);

	return NO_ERROR;
}

//内部保证数据是aligned的(DMA传输和计算都能达到较好的效果,虽然逻辑感觉会有问题，但是DMA在搬移的时候会剔除无用的数据)
void upScaleBy2_Cadence(upScaleBy2_Context *ctx , int phase)
{
	xvTile *pTileIn  = ctx->tileIn[phase];
	xvTile *pTileOut = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileIn);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileIn);
	int height  = XI_TILE_GET_HEIGHT(pTileIn);

	//UV定义的时候为uint16_t,因此这里一次性读取两个数据
	xb_vec2Nx8U* psrc = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileIn);
	xb_vec2Nx8U* pdst = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileOut);

	xb_vec2Nx8U*  __restrict rsrc;
	xb_vec2Nx8U*  __restrict rdst;

	int i , j = 0;
	valign a_st = IVP_ZALIGN();
	for( ; j < width -  XCHAL_IVPN_SIMD_WIDTH ; j += 2 * XCHAL_IVPN_SIMD_WIDTH , psrc += 1 , pdst += 2){

		rsrc = psrc;
		rdst = pdst;

		int offs = sstride - MIN(2 * XCHAL_IVPN_SIMD_WIDTH , width - j);
		int offd = dstride - MIN(4 * XCHAL_IVPN_SIMD_WIDTH , 2 * (width - j));
		//为了保证精度，只能将四个值累加后的结果再进行移位存储
		for(i = 0 ; i < height ; i++){
			xb_vec2Nx8U vsrc;
			xb_vec2Nx8U vTL , vTR , vBL , vBR;

			valign a_load = IVP_LA2NX8U_PP(rsrc);
			IVP_LAV2NX8U_XP(vsrc, a_load, rsrc,  width - j);
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);

			vTL = IVP_SEL2NX8UI(vsrc, vsrc, IVP_SELI_8B_INTERLEAVE_1_LO);
			vTR = IVP_SEL2NX8UI(vsrc, vsrc, IVP_SELI_8B_INTERLEAVE_1_HI);
			vBL = vTL;
			vBR = vTR;

			IVP_SAV2NX8U_XP(vTL, a_st, rdst, 2 * (width - j) );
			IVP_SAV2NX8U_XP(vTR, a_st, rdst, 2 * (width - j) - 2 * XCHAL_IVPN_SIMD_WIDTH );
			IVP_SAPOS2NX8U_FP(a_st , rdst);
			rdst = OFFSET_PTR_2NX8U(rdst, 1, offd, 0);

			IVP_SAV2NX8U_XP(vBL, a_st, rdst, 2 * (width - j) );
			IVP_SAV2NX8U_XP(vBR, a_st, rdst, 2 * (width - j) - 2 * XCHAL_IVPN_SIMD_WIDTH );
			IVP_SAPOS2NX8U_FP(a_st , rdst);
			rdst = OFFSET_PTR_2NX8U(rdst, 1, offd, 0);
		}
	}
	if(j < width){

		rsrc = psrc;
		rdst = pdst;

		int offs = sstride - (width - j);
		int offd = dstride - 2 * (width - j);
		for(i = 0 ; i < height ; i++){

			xb_vec2Nx8U vsrc;
			xb_vec2Nx8U vTop , vBottom;

			valign a_load = IVP_LA2NX8U_PP(rsrc);
			IVP_LAV2NX8U_XP(vsrc, a_load, rsrc,  width - j);
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);

			vTop = IVP_SEL2NX8UI(vsrc, vsrc, IVP_SELI_8B_INTERLEAVE_1_LO);
			vBottom = vTop;

			IVP_SAV2NX8U_XP(vTop, a_st, rdst, 2 * (width - j) );
			IVP_SAPOS2NX8U_FP(a_st , rdst);
			rdst = OFFSET_PTR_2NX8U(rdst, 1, offd, 0);

			IVP_SAV2NX8U_XP(vBottom, a_st, rdst, 2 * (width - j) );
			IVP_SAPOS2NX8U_FP(a_st , rdst);
			rdst = OFFSET_PTR_2NX8U(rdst, 1, offd, 0);
		}


	}
}


//内部保证数据是aligned的(DMA传输和计算都能达到较好的效果,虽然逻辑感觉会有问题，但是DMA在搬移的时候会剔除无用的数据)
void upScaleBy2_C(upScaleBy2_Context *ctx , int phase)
{
	xvTile *pTileIn  = ctx->tileIn[phase];
	xvTile *pTileOut = ctx->tileOut[phase];

	int sstride = XI_TILE_GET_PITCH(pTileIn);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileIn);
	int height  = XI_TILE_GET_HEIGHT(pTileIn);

	//UV定义的时候为uint16_t,因此这里一次性读取两个数据
	uint8_t* psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileIn);
	uint8_t* pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileOut);

	int i , j;
	for (j = 0; j < height; j++, psrc += sstride , pdst += (2 * dstride) ){

		for (i = 0; i < width; i++){

			int cp = psrc[i];

			pdst[2 * i] = cp;
			pdst[2 * i + 1] = cp;
			pdst[2 * i + dstride] = cp;
			pdst[2 * i + dstride + 1] = cp;
		}
	}
}



static ErrorType upScaleBy2_Process(upScaleBy2_Context *ctx)
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
	int numTiles = ((ctx->pInFrame->frameHeight + ctx->tileHeight - 1) / ctx->tileHeight)
			* ((ctx->pInFrame->frameWidth + ctx->tileWidth - 1) / ctx->tileWidth);

	// Tile processing loop
	int i , k;
	for (i = 0; i < numTiles; i++) {
		// Wait for the last input tile transfer to be finished
		WAIT_FOR_TILE_FAST(pTM, ctx->tileIn[bufIndex]);

		// Process the Buffer
		upScaleBy2_Cadence(ctx , bufIndex);

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
static ErrorType upScaleBy2_ReleaseContext(upScaleBy2_Context *ctx)
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

void upScaleBy2_U8(uint8_t *pIn , uint8_t *pOut , int width , int height , int stride)
{
	upScaleBy2_Context ctx;

	upScaleBy2_InitializeContext( &ctx , pIn , pOut , width , height , stride);

	upScaleBy2_Process(&ctx);

	upScaleBy2_ReleaseContext(&ctx);
}





