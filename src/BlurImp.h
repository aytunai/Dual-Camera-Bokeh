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
ErrorType BlurY_Level_2(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_3(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_4(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_4(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_5(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_6(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_7(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_8(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_9(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_10(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);

ErrorType BlurY_Level_11(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_12(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_13(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_14(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_15(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_16(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_17(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_18(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_19(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_20(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);

ErrorType BlurY_Level_21(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_22(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_23(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_24(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_25(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_26(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_27(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_28(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_29(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);
ErrorType BlurY_Level_30(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride);











ErrorType BlurUV_Level_1(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride);

ErrorType BlurUV_Level_2(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride);

ErrorType BlurUV_Level_3(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride);






#ifdef __cplusplus
}
#endif
















#endif /*  */
