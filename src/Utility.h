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


//通过这个来控制双核的CoreId(暂时先只出单核的版本，看最终的性能)
#define CORE_ID			0

#define ALIGNED_64B		64

#define XA_ALIGN(size, align) ((size + align - 1) & (~(align - 1)))

//global point for iDMA descriptor
extern idma_buffer_t *p_idma_2d_desc;

// DataRAM buffers
//#define DRAM_POOL_SIZE           (TILE_WIDTH * TILE_HEIGHT)
#define DRAM_POOL_SIZE           (100 * 1024)
extern uint8_t ALIGN64 pDramPool0[DRAM_POOL_SIZE] _LOCAL_RAM0_;
extern uint8_t ALIGN64 pDramPool1[DRAM_POOL_SIZE] _LOCAL_RAM1_;

#define VPU_LOG
//是否输出VPU的LOG信息
#ifdef	VPU_LOG
#define xa_printf(...)	printf(__VA_ARGS__)
#else
#define xa_printf(...)
#endif


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
