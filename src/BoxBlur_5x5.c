/*
 * BoxBlur_5x5.c
 *
 *  Created on: 2018-3-26
 *      Author: AiTong
 */

#include "stdio.h"
#include "BoxBlur_5x5.h"

//自定义的tile的大小
#define BOX_BLUR_TILE_WIDTH			256
#define BOX_BLUR_TILE_HEIGHT 		128
#define BOX_BLUR_TILE_PITCH		 	256

//定义放缩,1/25 * 2^17 = 5243，除数大一些，防止数据越界超过255
#define Q17_1div25     5243

//boxBlur的半径
#define BOX_BLUR_5x5_RADIUS			2

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

}BoxBlur_5x5_Context, *pBoxBlur_5x5_Context;

/*
 * Initialize context.
 */
static ErrorType BoxBlur5x5_InitializeContext(BoxBlur_5x5_Context *ctx , void *pSrc , int swidth , int sheight , int spitch ,
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

	ctx->srcTile.width  = BOX_BLUR_TILE_WIDTH;
	ctx->srcTile.height = BOX_BLUR_TILE_HEIGHT;
	//pitch must large or equal than (tile width + radius * 2)
	ctx->srcTile.pitch  = BOX_BLUR_TILE_PITCH + 2 * BOX_BLUR_5x5_RADIUS;
	ctx->dstTile.width  = BOX_BLUR_TILE_WIDTH;
	ctx->dstTile.height = BOX_BLUR_TILE_HEIGHT;
	ctx->dstTile.pitch  = BOX_BLUR_TILE_PITCH;

	ctx->tileWidth = BOX_BLUR_TILE_WIDTH , ctx->tileHeight = BOX_BLUR_TILE_HEIGHT;
	ctx->tilePitch = BOX_BLUR_TILE_PITCH;

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
	int tileInputBuffSize  = (ctx->srcTile.pitch + 2*BOX_BLUR_5x5_RADIUS) * (ctx->srcTile.height + 2*BOX_BLUR_5x5_RADIUS) + 64;
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
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, BOX_BLUR_5x5_RADIUS, BOX_BLUR_5x5_RADIUS, 0, 0, DATA_ALIGNED_64);
	SETUP_TILE(ctx->tileBuffer[1].pIn , ctx->pdramIn[1], tileInputBuffSize, ctx->pSrcFrame, ctx->srcTile.width,
			ctx->srcTile.height, ctx->srcTile.pitch, XV_TILE_U8, BOX_BLUR_5x5_RADIUS, BOX_BLUR_5x5_RADIUS, 0, 0, DATA_ALIGNED_64);

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
static inline ErrorType ScheduleInputDMA(BoxBlur_5x5_Context *ctx, xi_point *srcPos ,  xi_point *dstPos, TileBuffer *tileBuf)
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

//BoxBlur , size = 5x5,cadence版本
static ErrorType BoxBlur_5x5_U8_Cadence(TileBuffer *buffer)
{
	xvTile *pTileIn  = buffer->pIn;
	xvTile *pTileOut = buffer->pOut;

	int sstride = XI_TILE_GET_PITCH(pTileIn);
	int dstride = XI_TILE_GET_PITCH(pTileOut);
	int width   = XI_TILE_GET_WIDTH(pTileOut);
	int height  = XI_TILE_GET_HEIGHT(pTileOut);

	xb_vec2Nx8U *psrc = OFFSET_PTR_2NX8U( XI_TILE_GET_DATA_PTR(pTileIn) , -BOX_BLUR_5x5_RADIUS , sstride , -BOX_BLUR_5x5_RADIUS );
	xb_vec2Nx8U *pdst = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(pTileOut);

	xb_vec2Nx8U* __restrict rsrc;
	xb_vec2Nx8U* __restrict rdst;

	int i , j = 0;
	valign a_st = IVP_ZALIGN();
	for( ; j < width - XCHAL_IVPN_SIMD_WIDTH ; j += 2 * XCHAL_IVPN_SIMD_WIDTH , psrc++ , pdst++){

		rsrc = psrc;
		rdst = pdst;

        xb_vecNx16 suml  = IVP_ZERONX16();
        xb_vecNx16 sumh  = IVP_ZERONX16();
        xb_vecNx16 prevl = IVP_ZERONX16();
        xb_vecNx16 prevh = IVP_ZERONX16();

		int offs = sstride - MIN(4 * XCHAL_IVPN_SIMD_WIDTH , width + 4 - j);
		int offd = dstride - MIN(2 * XCHAL_IVPN_SIMD_WIDTH , width - j);

		xb_vecNx16 l0, h0;
        xb_vecNx16 l1, h1;
        xb_vecNx16 l2, h2;
        xb_vecNx16 l3, h3;

		#define xaBoxFilter_5x5_U8_load_row2N(i)                                                            \
		{                                                                                                   \
			valign a_load = IVP_LA2NX8U_PP (rsrc);                                                          \
			xb_vec2Nx8U vl; IVP_LAV2NX8U_XP(vl, a_load, rsrc, width - j + 4);                               \
			xb_vec2Nx8U vr; IVP_LAV2NX8U_XP(vr, a_load, rsrc, width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);   \
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);													\
			xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vr, vl, 0x01010101);                                          \
			IVP_ADDWUA2NX8(w, 0, IVP_SEL2NX8I(vr, vl, IVP_SELI_8B_ROTATE_RIGHT_4));                         \
			l##i = IVP_CVT16U2NX24L(w);                                                                     \
			h##i = IVP_CVT16U2NX24H(w);                                                                     \
			suml = IVP_ADDNX16(suml, l##i);                                                                 \
			sumh = IVP_ADDNX16(sumh, h##i);                                                                 \
		}

        xaBoxFilter_5x5_U8_load_row2N(0)
        xaBoxFilter_5x5_U8_load_row2N(1)
        xaBoxFilter_5x5_U8_load_row2N(2)
        xaBoxFilter_5x5_U8_load_row2N(3)

        for(i = 0 ; i < height ; i++){
			valign a_load = IVP_LA2NX8U_PP (rsrc);
			xb_vec2Nx8U vl; IVP_LAV2NX8U_XP(vl, a_load, rsrc, width - j + 4);
			xb_vec2Nx8U vr; IVP_LAV2NX8U_XP(vr, a_load, rsrc, width - j + 4 - 2 * XCHAL_IVPN_SIMD_WIDTH);
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);
			xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(vr, vl, 0x01010101);
			IVP_ADDWUA2NX8(w, 0, IVP_SEL2NX8I(vr, vl, IVP_SELI_8B_ROTATE_RIGHT_4));
			xb_vecNx16 nextl = IVP_CVT16U2NX24L(w);
			xb_vecNx16 nexth = IVP_CVT16U2NX24H(w);

            suml = IVP_ADDNX16(IVP_SUBNX16(nextl, prevl), suml);
            sumh = IVP_ADDNX16(IVP_SUBNX16(nexth, prevh), sumh);

            prevl = l0;    prevh = h0;
            l0    = l1;    h0    = h1;
            l1    = l2;    h1    = h2;
            l2    = l3;    h2    = h3;
            l3    = nextl; h3    = nexth;

            xb_vecNx16 dstl = IVP_PACKVRNX48(IVP_MULUUNX16(suml, Q17_1div25), 17);
            xb_vecNx16 dsth = IVP_PACKVRNX48(IVP_MULUUNX16(sumh, Q17_1div25), 17);
            xb_vec2Nx8U vdst = IVP_SEL2NX8UI(IVP_MOV2NX8U_FROMNX16(dsth), IVP_MOV2NX8U_FROMNX16(dstl), IVP_SELI_8B_EXTRACT_1_OF_2_OFF_0);
            IVP_SAV2NX8U_XP(vdst, a_st, rdst, width - j);
            IVP_SAPOS2NX8U_FP(a_st , rdst);
            rdst = OFFSET_PTR_2NX8U(rdst, 1, offd, 0);
        }
	}

	if(j < width){

		rsrc = psrc;
		xb_vecNx8U* __restrict rdst_2 = (xb_vecNx8U *)pdst;		//使用xb_vecNx8U的优势是在保存的时候可以避免使用SEL指令

		xb_vecNx16 suml  = IVP_ZERONX16();
		xb_vecNx16 prevl = IVP_ZERONX16();

		int offs = sstride - (width + 4 - j);
		int offd = dstride - (width - j);

		xb_vecNx16 l0 , l1 , l2 , l3;

		#define xaBoxFilter_5x5_U8_load_rowN(i)                                                            \
		{                                                                                                  \
			valign a_load = IVP_LA2NX8U_PP (rsrc);                                                         \
			xb_vec2Nx8U vl; IVP_LAV2NX8U_XP(vl, a_load, rsrc, width - j + 4);                              \
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);												   \
			xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(0, vl, 0x01010101);                                          \
			IVP_ADDWUA2NX8(w, 0, IVP_SEL2NX8I(0, vl, IVP_SELI_8B_ROTATE_RIGHT_4));                         \
			l##i = IVP_CVT16U2NX24L(w);                                                                    \
			suml = IVP_ADDNX16(suml, l##i);                                                                \
		}

		xaBoxFilter_5x5_U8_load_rowN(0)
		xaBoxFilter_5x5_U8_load_rowN(1)
		xaBoxFilter_5x5_U8_load_rowN(2)
		xaBoxFilter_5x5_U8_load_rowN(3)

        for(i = 0 ; i < height ; i++){
			valign a_load = IVP_LA2NX8U_PP (rsrc);
			xb_vec2Nx8U vl; IVP_LAV2NX8U_XP(vl, a_load, rsrc, width - j + 4);
			rsrc = OFFSET_PTR_2NX8U(rsrc , 1 , offs , 0);
			xb_vec2Nx24 w = IVP_MULUS4T2N8XR8(0, vl, 0x01010101);
			IVP_ADDWUA2NX8(w, 0, IVP_SEL2NX8I(0, vl, IVP_SELI_8B_ROTATE_RIGHT_4));
			xb_vecNx16 nextl = IVP_CVT16U2NX24L(w);

            suml = IVP_ADDNX16(IVP_SUBNX16(nextl, prevl), suml);

            prevl = l0;
            l0    = l1;
            l1    = l2;
            l2    = l3;
            l3    = nextl;

            xb_vecNx16 vdst = IVP_PACKVRNX48(IVP_MULUUNX16(suml, Q17_1div25), 17);

			IVP_SAVNX8U_XP(vdst, a_st, rdst_2, width - j);
			IVP_SAPOSNX8U_FP(a_st , rdst_2);
			rdst_2 = OFFSET_PTR_NX8U(rdst_2 , 1 , offd , 0);
        }
	}

	return NO_ERROR;
}
/*
 * Process context.
 */
static ErrorType BoxBlur_5x5_Process(BoxBlur_5x5_Context *ctx)
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
	int numTiles = ((ctx->pOutFrame->frameHeight + ctx->tileHeight - 1) / ctx->tileHeight)
			* ((ctx->pOutFrame->frameWidth + ctx->tileWidth - 1) / ctx->tileWidth);

	// Tile processing loop
	int i;
	for (i = 0; i < numTiles; i++) {
		// Get current double-buffer for processing
		TileBuffer *pTileBuffer = &ctx->tileBuffer[bufIndex];

		// Wait for the input tile transfer to be finished
		WAIT_FOR_TILE(pTM, pTileBuffer->pIn);

		// Process the Buffer
		BoxBlur_5x5_U8_Cadence(pTileBuffer);

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
	xa_printf("Done\n");
}


/*
 * Release context.
 */
static ErrorType BoxBlur_5x5_ReleaseContext(BoxBlur_5x5_Context *ctx)
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
void BoxBlur_5x5_U8(uint8_t *src , uint8_t *dst , int width , int height , int pitch)
{

	BoxBlur_5x5_Context ctx;

	BoxBlur5x5_InitializeContext(&ctx , src , width , height , pitch ,dst , width , height , pitch);


	BoxBlur_5x5_Process(&ctx);

	BoxBlur_5x5_ReleaseContext(&ctx);

}









