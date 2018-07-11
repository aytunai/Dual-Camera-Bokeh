/*
 * UpDepthByAvg.c
 *
 *  Created on: 2018-3-26
 *      Author: AiTong
 */

#include "stdio.h"
#include "UpDepthByAvg.h"

//自定义的tile的大小
#define UP_DEPTH_BY_AVG_TILE_WIDTH			128
#define UP_DEPTH_BY_AVG_TILE_HEIGHT 		128
#define UP_DEPTH_BY_AVG_TILE_PITCH		 	128


//boxBlur的半径
#define UP_DEPTH_BY_AVG_3x3_RADIUS		1

/*
 * Context data structure.
 *
 * This data structure holds all parameters and global variables which are
 * shared between different modules. This data structure will reside in the
 * stack in the DataRAM.
 */
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
	xvFrame *pSrcFrame;    		// source image
	xvFrame *pOutFrame;     	// output image

	//DataRAM double-buffer(tile buffer)(ping-pong buffer)
	TileBuffer tileBuffer[2];
	uint8_t *pdramIn[2];		// pointer of input  double-buffer in DataRAM(return allocate origin pointer)
	uint8_t *pdramOut[2];		// pointer of output double-buffer in DataRAM(return allocate origin pointer)

	//局部变量使用,记录tile的信息,都保存在Ctx结构体当中
	xi_tile srcTile;          	// source tile info
	xi_tile dstTile;          	// output tile info

	// runtime related
	xi_point srcPos[2];			//更新当前Tile的位置
	xi_point dstPos[2];

}UpDepthByAvg_Context, *pUpDepthByAvg_Context;

/*
 * Initialize context.
 */
static ErrorType UpDepthByAvg_InitializeContext(UpDepthByAvg_Context *ctx , void *pSrc , int swidth , int sheight , int spitch ,
		void *pDst , int dwidth , int dheight , int dpitch)
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

	ctx->srcTile.width  = UP_DEPTH_BY_AVG_TILE_WIDTH;
	ctx->srcTile.height = UP_DEPTH_BY_AVG_TILE_HEIGHT;
	//pitch must large or equal than (tile width + radius * 2)
	ctx->srcTile.pitch  = UP_DEPTH_BY_AVG_TILE_PITCH + 2 * UP_DEPTH_BY_AVG_3x3_RADIUS;
	ctx->dstTile.width  = UP_DEPTH_BY_AVG_TILE_WIDTH  * 2;
	ctx->dstTile.height = UP_DEPTH_BY_AVG_TILE_HEIGHT * 2;
	ctx->dstTile.pitch  = UP_DEPTH_BY_AVG_TILE_PITCH  * 2;

	ctx->tileWidth = UP_DEPTH_BY_AVG_TILE_WIDTH , ctx->tileHeight = UP_DEPTH_BY_AVG_TILE_HEIGHT;
	ctx->tilePitch = UP_DEPTH_BY_AVG_TILE_PITCH;

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
	ctx->pSrcFrame = xvAllocateFrame(pTM);
	ctx->pOutFrame = xvAllocateFrame(pTM);
	if ((int32_t) ctx->pSrcFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pOutFrame == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_FRAME(ctx->pSrcFrame, pSrc, sframeSize, swidth, sheight, spitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);
	SETUP_FRAME(ctx->pOutFrame, pDst, dframeSize, dwidth, dheight, dpitch, 0, 0, 1, 1, FRAME_EDGE_PADDING, 0);

	// Allocate input tile double-buffer. Here we allocate the double buffer 0 and 1 in
	// different DataRAMs, to avoid access conflict between them.
	// add 64bytes in order to aligned with 64 bytes
	int tileInputBuffSize  = (ctx->srcTile.pitch) * (ctx->srcTile.height + 2 * UP_DEPTH_BY_AVG_3x3_RADIUS) + 64;
	int tileOutputBuffSize = (ctx->dstTile.pitch) * (ctx->dstTile.height);

	// Allocate Input tile 0/1 in DataRAM 0/1
	ctx->pdramIn[0] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramIn[1] = xvAllocateBuffer(pTM, tileInputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramIn[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramIn[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileBuffer[0].pIn = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileBuffer[0].pIn == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileBuffer[1].pIn = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileBuffer[1].pIn == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileBuffer[0].pIn , ctx->pdramIn[0], tileInputBuffSize, ctx->pSrcFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, UP_DEPTH_BY_AVG_3x3_RADIUS, UP_DEPTH_BY_AVG_3x3_RADIUS, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileBuffer[1].pIn , ctx->pdramIn[1], tileInputBuffSize, ctx->pSrcFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, UP_DEPTH_BY_AVG_3x3_RADIUS, UP_DEPTH_BY_AVG_3x3_RADIUS, 0, 0, DATA_ALIGNED_64);

	// Allocate Output tile 0/1 in DataRAM 0/1
	ctx->pdramOut[0] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_0, 64);
	ctx->pdramOut[1] = xvAllocateBuffer(pTM, tileOutputBuffSize, XV_MEM_BANK_COLOR_1, 64);
	if ((int32_t) ctx->pdramOut[0] == XVTM_ERROR) {
		return TM_ERROR;
	}
	if ((int32_t) ctx->pdramOut[1] == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileBuffer[0].pOut = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileBuffer[0].pOut == XVTM_ERROR) {
		return TM_ERROR;
	}
	ctx->tileBuffer[1].pOut = xvAllocateTile(pTM);
	if ((int32_t) ctx->tileBuffer[1].pOut == XVTM_ERROR) {
		return TM_ERROR;
	}
	SETUP_TILE(ctx->tileBuffer[0].pOut , ctx->pdramOut[0], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
			ctx->dstTile.height, ctx->dstTile.pitch, XV_TILE_U8, 0, 0, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileBuffer[1].pOut , ctx->pdramOut[1], tileOutputBuffSize, ctx->pOutFrame, ctx->dstTile.width,
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
 * tileBuf - pointer of current tile buffer. A tile buffer contains one input tile and
 *           one output tile. The processing kernel runs on a tile buffer, applying
 *           affine transform on the input tile and write the result to the output tile.
 */
static inline ErrorType ScheduleInputDMA(UpDepthByAvg_Context *ctx, xi_point *srcPos ,  xi_point *dstPos, TileBuffer *tileBuf)
{
	xvTileManager *pTM = &ctx->tileMgr;

	xi_size srcSize, dstSize;

	// Compute input tile position and size using output tile position and size.
	dstSize.width 	= MAX(0, MIN(ctx->dstTile.width,  ctx->pOutFrame->frameWidth  - dstPos->x));
	dstSize.height 	= MAX(0, MIN(ctx->dstTile.height, ctx->pOutFrame->frameHeight - dstPos->y));
	srcSize.width 	= MAX(0, MIN(ctx->srcTile.width,  ctx->pSrcFrame->frameWidth  - srcPos->x));
	srcSize.height 	= MAX(0, MIN(ctx->srcTile.height, ctx->pSrcFrame->frameHeight - srcPos->y));

	// Update output tile parameters
	XV_TILE_UPDATE_DIMENSIONS(tileBuf->pOut, dstPos->x, dstPos->y, dstSize.width, dstSize.height, ctx->dstTile.pitch);

	int tw = ((srcPos->x + ctx->srcTile.width)  >= ctx->pSrcFrame->frameWidth)  ? (srcSize.width):(ctx->srcTile.width);
	int th = ((srcPos->y + ctx->srcTile.height) >= ctx->pSrcFrame->frameHeight) ? (srcSize.height):(ctx->srcTile.height);
	// Update input tile parameters and issue the DMA request
	XV_TILE_UPDATE_DIMENSIONS(tileBuf->pIn, srcPos->x, srcPos->y, tw,th, ctx->srcTile.pitch);

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

	//Transfer data from external memory bank to Local DRAM
	int32_t retVal = xvReqTileTransferIn(pTM, tileBuf->pIn , NULL , 0);
	if (retVal == XVTM_ERROR) {
		return TM_ERROR;
	}

	return NO_ERROR;
}

#define xaUpscaleByAvg_row2N__1(vec0 , vec1)														\
		{																							\
			xb_vec2Nx8U vsel0 , vtail;																\
			valign a_load = IVP_LA2NX8U_PP(rsrc);													\
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , sstride , 0);										\
			IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, swidth + 1 - j);									\
			IVP_LAV2NX8U_XP(vtail, a_load, rsrc, swidth + 1 - j - 2 * XCHAL_IVPN_SIMD_WIDTH);		\
			rsrc = bsrc;																			\
			vnext = IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);							\
			vecTmp = IVP_AVGRU2NX8U(vsel0, vnext);													\
			IVP_DSEL2NX8UI(vec1, vec0, vecTmp, vsel0, IVP_DSELI_8B_INTERLEAVE_1);					\
			IVP_SAV2NX8U_XP(vec0, a_store, rdste, dwidth - 2 * j);									\
			IVP_SAV2NX8U_XP(vec1, a_store, rdste, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);		\
			IVP_SAPOS2NX8U_FP(a_store , rdste);														\
			rdste = OFFSET_PTR_2NX8U(rdste , 1 , dstride , offd);									\
		}



//depth UpSample By Average
static ErrorType UpDepthByAvg_U8_Cadence(TileBuffer *buffer)
{
	xvTile *pTileIn  = buffer->pIn;
	xvTile *pTileOut = buffer->pOut;

	int sstride = XI_TILE_GET_PITCH(pTileIn);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int swidth  = XI_TILE_GET_WIDTH(pTileIn);
	int sheight = XI_TILE_GET_HEIGHT(pTileIn);
	int dwidth  = XI_TILE_GET_WIDTH(pTileOut);
	int dheight = XI_TILE_GET_HEIGHT(pTileOut);

	xb_vec2Nx8U *psrc = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileIn);
	xb_vec2Nx8U *pdst = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileOut);

	xb_vec2Nx8U* __restrict rsrc;
	xb_vec2Nx8U* __restrict bsrc;
	xb_vec2Nx8U* __restrict rdsto;
	xb_vec2Nx8U* __restrict rdste;

	xb_vec2Nx8U vnext;
	xb_vec2Nx8U vecTmp;

	xb_vec2Nx8U vec00 , vec01 , vec10 , vec11 , vec20 , vec21;

	int i , j = 0;
	valign a_store = IVP_ZALIGN();
	for( ; j < swidth - XCHAL_IVPN_SIMD_WIDTH ; j += 2 * XCHAL_IVPN_SIMD_WIDTH , psrc++ , pdst +=2){

		int offd = dstride - MIN(4 * XCHAL_IVPN_SIMD_WIDTH , dwidth - 2 * j);

		rsrc  = psrc;
		rdste = pdst;
		rdsto = OFFSET_PTR_2NX8U(rdste , 1 , dstride , 0);		//偏移一行
		//下面这个的结果将会保存在偶数行
#define xaUpscaleByAvg_row2N(vec0 , vec1)															\
		{																							\
			xb_vec2Nx8U vsel0 , vtail;																\
			valign a_load = IVP_LA2NX8U_PP(rsrc);													\
			bsrc = OFFSET_PTR_2NX8U(rsrc , 1 , sstride , 0);										\
			IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, swidth + 1 - j);									\
			IVP_LAV2NX8U_XP(vtail, a_load, rsrc, swidth + 1 - j - 2 * XCHAL_IVPN_SIMD_WIDTH);		\
			rsrc = bsrc;																			\
			vnext = IVP_SEL2NX8I(vtail, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);							\
			vecTmp = IVP_AVGRU2NX8U(vsel0, vnext);													\
			IVP_DSEL2NX8UI(vec1, vec0, vecTmp, vsel0, IVP_DSELI_8B_INTERLEAVE_1);					\
			IVP_SAV2NX8U_XP(vec0, a_store, rdste, dwidth - 2 * j);									\
			IVP_SAV2NX8U_XP(vec1, a_store, rdste, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);		\
			IVP_SAPOS2NX8U_FP(a_store , rdste);														\
			rdste = OFFSET_PTR_2NX8U(rdste , 1 , dstride , offd);									\
		}

		xaUpscaleByAvg_row2N(vec00 , vec01);
		//Warning: top的那一条边是无效的
		for(i = 0 ; i < sheight ; i++){

			xaUpscaleByAvg_row2N(vec20,vec21);

			vec10 = IVP_AVGRU2NX8U(vec00, vec20);
			vec11 = IVP_AVGRU2NX8U(vec01, vec21);

			IVP_SAV2NX8U_XP(vec10, a_store, rdsto, dwidth - 2 * j);
			IVP_SAV2NX8U_XP(vec11, a_store, rdsto, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);
			IVP_SAPOS2NX8U_FP(a_store , rdsto);
			rdsto = OFFSET_PTR_2NX8U(rdsto , 1 , dstride , offd);

			vec00 = vec20;
			vec01 = vec21;
		}
		//最后一行需要特别处理
		vec10 = IVP_AVGRU2NX8U(vec00, vec20);
		vec11 = IVP_AVGRU2NX8U(vec01, vec21);

		IVP_SAV2NX8U_XP(vec10, a_store, rdsto, dwidth - 2 * j);
		IVP_SAV2NX8U_XP(vec11, a_store, rdsto, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);
		IVP_SAPOS2NX8U_FP(a_store , rdsto);
		rdsto = OFFSET_PTR_2NX8U(rdsto , 1 , dstride , offd);
	}

	if(j < swidth + 1){

		int offd = dstride - (dwidth - 2 * j);

		rsrc  = psrc;
		rdste = pdst;
		rdsto = OFFSET_PTR_2NX8U(rdste , 1 , dstride , 0);		//偏移一行
		//下面这个的结果将会保存在偶数行
#define xaUpscaleByAvg_rowN(vec0,vec1)																\
		{																							\
			xb_vec2Nx8U vsel0 ;																		\
			valign a_load = IVP_LA2NX8U_PP(rsrc);													\
			bsrc = OFFSET_PTR_2NX8U(rsrc , 1 , sstride , 0);										\
			IVP_LAV2NX8U_XP(vsel0, a_load, rsrc, swidth + 1 - j);									\
			rsrc = bsrc;																			\
			vnext = IVP_SEL2NX8I(vsel0, vsel0, IVP_SELI_8B_ROTATE_RIGHT_1);							\
			vecTmp = IVP_AVGRU2NX8U(vsel0, vnext);													\
			IVP_DSEL2NX8UI(vec1, vec0, vecTmp, vsel0, IVP_DSELI_8B_INTERLEAVE_1);					\
			IVP_SAV2NX8U_XP(vec0, a_store, rdste, dwidth - 2 * j);									\
			IVP_SAV2NX8U_XP(vec1, a_store, rdste, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);		\
			IVP_SAPOS2NX8U_FP(a_store , rdste);														\
			rdste = OFFSET_PTR_2NX8U(rdste , 1 , dstride , offd);									\
		}

		xaUpscaleByAvg_rowN(vec00 , vec01);

		//Warning: top的那一条边是无效的
		for(i = 0 ; i < sheight ; i++){

			xaUpscaleByAvg_rowN(vec20 , vec21);

			vec10 = IVP_AVGRU2NX8U(vec00, vec20);
			vec11 = IVP_AVGRU2NX8U(vec01, vec21);
			vec00 = vec20;
			vec01 = vec21;
			IVP_SAV2NX8U_XP(vec10, a_store, rdsto, dwidth - 2 * j);
			IVP_SAV2NX8U_XP(vec11, a_store, rdsto, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);
			IVP_SAPOS2NX8U_FP(a_store , rdsto);
			rdsto = OFFSET_PTR_2NX8U(rdsto , 1 , dstride , offd);
		}
		//最后一行需要特别处理
		vec10 = IVP_AVGRU2NX8U(vec00, vec20);
		vec11 = IVP_AVGRU2NX8U(vec01, vec21);

		IVP_SAV2NX8U_XP(vec10, a_store, rdsto, dwidth - 2 * j);
		IVP_SAV2NX8U_XP(vec11, a_store, rdsto, dwidth - 2 * j - 2 * XCHAL_IVPN_SIMD_WIDTH);
		IVP_SAPOS2NX8U_FP(a_store , rdsto);
		rdsto = OFFSET_PTR_2NX8U(rdsto , 1 , dstride , offd);
	}


	return NO_ERROR;
}

//Y-component boxBlur , size = 3x3,C版本
static ErrorType UpDepthByAvg_U8_C(TileBuffer *buffer)
{
	xvTile *pTileIn  = buffer->pIn;
	xvTile *pTileOut = buffer->pOut;

	int sstride = XI_TILE_GET_PITCH(pTileIn);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileIn);
	int height  = XI_TILE_GET_HEIGHT(pTileIn);

	uint8_t *psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileIn);
	uint8_t *pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(pTileOut);



	return NO_ERROR;
}

/*
 * Process context.
 */
static ErrorType UpDepthByAvg_Process(UpDepthByAvg_Context *ctx)
{
	xvTileManager *pTM = &ctx->tileMgr; // get pointer of TileManager
	int32_t bufIndex = 0; // this is the double-buffer index
	xi_point srcPos = {0,0}; // this is used to track input tile position
	xi_point dstPos = {0,0}; // this is used to track output tile position

	ErrorType error;

	// Schedule 2 input DMA transfers at the beginning to hide the initial DMA transfer latency
	error = ScheduleInputDMA(ctx, &srcPos , &dstPos, &ctx->tileBuffer[0]);
	error = ScheduleInputDMA(ctx, &srcPos , &dstPos, &ctx->tileBuffer[1]);

	if (error != NO_ERROR) {
#pragma frequency_hint never
		goto errorHandler;
	}

	// Calculate how many tiles we need to process
	int numTiles = ((ctx->pSrcFrame->frameHeight + ctx->tileHeight - 1) / ctx->tileHeight)
			* ((ctx->pSrcFrame->frameWidth + ctx->tileWidth - 1) / ctx->tileWidth);

	// Tile processing loop
	int i;
	for (i = 0; i < numTiles; i++) {

		// Get current double-buffer for processing
		TileBuffer *pTileBuffer = &ctx->tileBuffer[bufIndex];

		// Wait for the input tile transfer to be finished
		WAIT_FOR_TILE(pTM, pTileBuffer->pIn);

		// Process the Buffer
#ifdef CADENCE_OPT_UP_DEPTH_BY_AVG
		UpDepthByAvg_U8_Cadence(pTileBuffer);
#else
		UpDepthByAvg_U8_C(pTileBuffer);

#endif

		// Schedule output DMA
		int32_t error1 = xvReqTileTransferOut(pTM, pTileBuffer->pOut , 0);
		if (error1 != XVTM_SUCCESS) {
#pragma frequency_hint never
			error = TM_ERROR;
			break;
		}

		// Schedule next input DMA
		error = ScheduleInputDMA(ctx, &srcPos , &dstPos, pTileBuffer);
		if (error != NO_ERROR) {
#pragma frequency_hint never
			break;
		}

		// Toggle double-buffer index
		bufIndex ^= 0x1;
	}
	//后面还需要再拷贝回来一份数据
	// Wait for the last output tile transfer to be finished
	WAIT_FOR_TILE(pTM, ctx->tileBuffer[bufIndex ^ 1].pOut);

errorHandler:
	printf("Done\n");
}


/*
 * Release context.
 */
static ErrorType UpDepthByAvg_ReleaseContext(UpDepthByAvg_Context *ctx)
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


//BoxBlur for Y-component
void UpDepthByAvg_U8(uint8_t *src , uint8_t *dst , int width , int height , int pitch)
{

	UpDepthByAvg_Context ctx;

	int dwidth  = width  * 2;
	int dheight = height * 2;
	int dpitch  = pitch * 2;

	UpDepthByAvg_InitializeContext(&ctx , src , width , height , pitch ,dst , dwidth , dheight , dpitch);


	UpDepthByAvg_Process(&ctx);

	UpDepthByAvg_ReleaseContext(&ctx);

}









