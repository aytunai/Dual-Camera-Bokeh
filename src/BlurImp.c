/*
 * BlurImp.c
 *
 *  Created on: 2018-7-11
 *      Author: AI
 */

#include "BlurImp.h"

ErrorType BlurY_Level_1(uint8_t *pSrc , uint8_t *pDst , int width , int height , int stride)
{
	downScaleBy2_U8(pSrc , pDst , width , height , stride);
}

ErrorType BlurUV_Level_1(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride)
{
	//Use Memory Pool
	uint8_t *pBufU = pBuf;
	uint8_t *pBufV = pBufU + height * stride;

	uint32_t MemoryUsePtr = (2 * height * stride);
	xa_printf("BlurUV_Level_1 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);


	BoxBlur_3x3_U8(psrcU , pBufU , width , height , stride);
	BoxBlur_3x3_U8(pBufU , pdstU , width , height , stride);
	BoxBlur_3x3_U8(psrcV , pBufV , width , height , stride);
	BoxBlur_3x3_U8(pBufV , pdstV , width , height , stride);
}












#if 0
ErrorType SeparateUV(uint8_t *pSrcUV , uint8_t *pDstU , uint8_t *pDstV , uint8_t **pGBuf , int width , int height , int stride)
{
	uint8_t *pBuf = *pGBuf;


	//占用的内存空间
	*pGBuf += (2 * height * stride);
}
#endif



