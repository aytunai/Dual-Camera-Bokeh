/*
 * BlurImp.h
 *
 *  Created on: 2018-7-11
 *      Author: AI
 */

#ifndef _BLURIMP_H
#define _BLURIMP_H

#include "typedef.h"
#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"

#include "Separate.h"
#include "DownScaleBy2.h"
#include "BoxBlur_3x3.h"

//--------------------------Function----------------------------//
#ifdef __cplusplus
extern "C" {
#endif


ErrorType BlurY_Level_1(uint8_t *pSrc , uint8_t *pDst , int width , int height , int stride);

ErrorType BlurUV_Level_1(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride);








#ifdef __cplusplus
}
#endif
















#endif /*  */
