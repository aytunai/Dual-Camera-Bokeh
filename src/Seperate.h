/*
 * Seperate.h
 *
 *  Created on: 2018-4-11
 *      Author: AiTong
 */

#ifndef _SEPERATE_H
#define _SEPERATE_H

#include <xtensa/tie/xt_ivpn.h>
#include "Utility.h"



#define SEPERATE_OPT_CADENCE


#ifdef __cplusplus
extern "C" {
#endif

//UV�����ݸĳ�uint16��Ϊ���������ݵĴ�С��һ�µ�����
void UV_Seperate_U8(uint8_t *pUV , uint8_t *pU , uint8_t *pV , int width , int height , int stride);


#ifdef __cplusplus
}
#endif



#endif /*  */
