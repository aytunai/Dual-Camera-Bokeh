/*
 * Utility.h
 *
 *  Created on: 2018-3-23
 *      Author: AiTong
 */

#ifndef _UTILITY_H
#define _UTILITY_H

#include "tileManager.h"
#include "typedef.h"
#include "common.h"
#include "xi_pointer_translation.h"
#include "Debug.h"

#define ALIGNED_64B		64

//global point for iDMA descriptor
extern idma_buffer_t *p_idma_2d_desc;

// DataRAM buffers
//#define DRAM_POOL_SIZE           (TILE_WIDTH * TILE_HEIGHT)
#define DRAM_POOL_SIZE           (100 * 1024)
extern uint8_t ALIGN64 pDramPool0[DRAM_POOL_SIZE] _LOCAL_RAM0_;
extern uint8_t ALIGN64 pDramPool1[DRAM_POOL_SIZE] _LOCAL_RAM1_;




//--------------------------Function----------------------------//
#ifdef __cplusplus
extern "C" {
#endif

void errCallbackFunc(idma_error_details_t* data);


void intrCallbackFunc(void *pCallbackData);




#ifdef __cplusplus
}
#endif














#endif /*  */
