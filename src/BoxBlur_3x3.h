/*
 * BoxBlur_3x3.h
 *
 *  Created on: 2018-3-26
 *      Author: AiTong
 */

#ifndef _BOX_BLUR_3X3_H
#define _BOX_BLUR_3X3_H


#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"


#define CADENCE_OPT_BOX_BLUR_3x3



#ifdef __cplusplus
extern "C" {
#endif


void BoxBlur_3x3_U8(uint8_t *src , uint8_t *dst , int width , int height , int stride);


#ifdef __cplusplus
}
#endif







#endif /*  */
