/*
 * UpScaleBy2.h
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#ifndef _UP_SCALE_BY2_H
#define _UP_SCALE_BY2_H

#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"



#define UPSCALEBY2_OPT_CADENCE


#ifdef __cplusplus
extern "C" {
#endif

//UV�����ݸĳ�uint16��Ϊ���������ݵĴ�С��һ�µ�����
void UpScaleBy2_U8(uint8_t *pIn , uint8_t *pOut , int width , int height , int stride);


#ifdef __cplusplus
}
#endif



#endif /*  */
