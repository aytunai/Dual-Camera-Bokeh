/*
 * Combine.h
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#ifndef _COMBINE_H
#define _COMBINE_H

#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"

#define COMBINE_CADENCE_OPT


#ifdef __cplusplus
extern "C" {
#endif

//UV的数据改成uint16是为了限制数据的大小不一致的问题
void UV_Combine_U8(uint8_t *pUV , uint8_t *pU , uint8_t *pV , int width , int height , int stride);


#ifdef __cplusplus
}
#endif



#endif /*  */
