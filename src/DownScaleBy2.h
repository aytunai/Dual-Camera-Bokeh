/*
 * DownScaleBy2.h
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#ifndef _DOWN_SCALE_BY2_H
#define _DOWN_SCALE_BY2_H

#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"



#define DOWNSCALEBY2_OPT_CADENCE


#ifdef __cplusplus
extern "C" {
#endif

//UV的数据改成uint16是为了限制数据的大小不一致的问题
void downScaleBy2_U8(uint8_t *pIn , uint8_t *pOut , int width , int height , int stride);


#ifdef __cplusplus
}
#endif



#endif /*  */
