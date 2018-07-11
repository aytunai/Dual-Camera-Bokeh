/*
 * UpDepthByAvg.h
 *
 *  Created on: 2018-3-26
 *      Author: AiTong
 */

#ifndef _UP_DEPTH_BY_AVG_H
#define _UP_DEPTH_BY_AVG_H


#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"


#define CADENCE_OPT_UP_DEPTH_BY_AVG



#ifdef __cplusplus
extern "C" {
#endif


void UpDepthByAvg_U8(uint8_t *src , uint8_t *dst , int width , int height , int stride);


#ifdef __cplusplus
}
#endif







#endif /*  */
