/*
 * Separate.h
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#ifndef _SEPARATE_H
#define _SEPARATE_H

#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"



#define SEPARATE_OPT_CADENCE


#ifdef __cplusplus
extern "C" {
#endif

//UV�����ݸĳ�uint16��Ϊ���������ݵĴ�С��һ�µ�����
void UV_Separate_U8(uint8_t *pUV , uint8_t *pU , uint8_t *pV , int width , int height , int stride);


#ifdef __cplusplus
}
#endif



#endif /*  */
