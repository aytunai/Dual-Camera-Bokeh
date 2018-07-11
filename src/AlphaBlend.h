/*
 * AlphaBlendY.h
 *
 *  Created on: 2018-3-26
 *      Author: AiTong
 */

#ifndef _ALPHA_BLEND_Y_H
#define _ALPHA_BLEND_Y_H


#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"


#define CADENCE_OPT_ALPHA_BLEND



#ifdef __cplusplus
extern "C" {
#endif

//width,height,pitch都是表示Y的参数(因此depth的width,height,pitch的值实际上为Y的一半)
void AlphaBlend_U8(uint8_t *pSrc , uint8_t *pBlur , uint8_t *pDepth , uint8_t *pOut , int width , int height , int pitch);


#ifdef __cplusplus
}
#endif







#endif /*  */
