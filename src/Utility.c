/*
 * Utility.c
 *
 *  Created on: 2018-3-23
 *      Author: AiTong
 */

#include "Utility.h"

// Declare and allocate iDMA object
IDMA_BUFFER_DEFINE(idmaObj, DMA_DESCR_CNT, IDMA_2D_DESC);
idma_buffer_t *p_idma_2d_desc = idmaObj;

// DataRAM buffers
//#define DRAM_POOL_SIZE           (TILE_WIDTH * TILE_HEIGHT)
#define DRAM_POOL_SIZE           (100 * 1024)
uint8_t ALIGN64 pDramPool0[DRAM_POOL_SIZE] _LOCAL_RAM0_;
uint8_t ALIGN64 pDramPool1[DRAM_POOL_SIZE] _LOCAL_RAM1_;

/*
 * IDMA error callback function
 */
void errCallbackFunc(idma_error_details_t* data)
{
	printf("ERROR CALLBACK: iDMA in Error\n");
#if defined(__XTENSA__)
	printf("COPY FAILED, Error %d at desc:%p, PIF src/dst = %x/%x\n",
			data->err_type, (void *) data->currDesc, data->srcAddr, data->dstAddr);
#endif
	return;
}

/*
 * IDMA Interrupt callback function
 */
void intrCallbackFunc(void *pCallbackData)
{
	Context *ctx = (Context *)pCallbackData;
	ctx->intrCount++;
}

