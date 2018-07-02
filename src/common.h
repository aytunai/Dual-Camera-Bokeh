/*
 * common.h
 *
 *  Created on: 2018-3-23
 *      Author: AiTong
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "stdio.h"
#include "stdint.h"

#define MAX(x,y) ((x)<(y) ? (y) : (x))
#define MIN(x,y) ((x)>(y) ? (y) : (x))

// Error type code
typedef enum {
	NO_ERROR = 0,
	TM_ERROR,
	XILIB_ERROR,
	MEMORY_ERROR,
	OTHER_ERROR,
} ErrorType;

//YUV数据格式
typedef struct {
	uint8_t *pYUV;
	uint8_t *pY;
	uint8_t *pU;
	uint8_t *pV;
	uint8_t *pUV;
}yuv_t;



#endif /*  */
