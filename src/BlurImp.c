/*
 * BlurImp.c
 *
 *  Created on: 2018-7-11
 *      Author: AI
 */

#include "BlurImp.h"

//*******************************************************************************************************************//
/*												BlurY																 */
//*******************************************************************************************************************//
ErrorType BlurY_Level_1(uint8_t *pSrc , uint8_t *pDst , int width , int height , int stride)
{
	downScaleBy2_U8(pSrc , pDst , width , height , stride);
}

ErrorType BlurY_Level_2(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride);
	xa_printf("BlurUV_Level_2 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_3(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride);
	xa_printf("BlurUV_Level_3 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_4(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride);
	xa_printf("BlurUV_Level_4 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);

}

ErrorType BlurY_Level_5(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_5 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_6(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_6 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_7(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_7 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_8(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride);
	xa_printf("BlurUV_Level_8 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride * 2);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_9(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_9 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_10(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_10 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_11(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurUV_Level_11 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_12(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurY_Level_12 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_13(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 2);
	xa_printf("BlurY_Level_13 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf1 = pBuf;
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf1 , width , height , stride);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_3x3_U8(buf1  , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_14(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_14 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	GaussBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_15(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_15 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_16(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_16 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	GaussBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_17(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_17 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_18(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_18 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_19(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_19 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	GaussBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_20(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_20 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_21(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_21 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_22(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_22 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_23(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_23 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_24(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_24 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_25(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_25 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_26(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_26 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_27(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_27 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}

ErrorType BlurY_Level_28(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_28 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf2 , buf1 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf1 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_29(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_29 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);

	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}


ErrorType BlurY_Level_30(uint8_t *pSrc , uint8_t *pDst , uint8_t *pBuf ,int width , int height , int stride)
{
	uint32_t MemoryUsePtr = (height * stride * 3);
	xa_printf("BlurY_Level_30 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);

	uint8_t *buf0 = pBuf;
	uint8_t *buf1 = buf0 + (height * stride);
	uint8_t *buf2 = buf1 + (height * stride);

	downScaleBy2_U8(pSrc , buf0 , width , height , stride);
	downScaleBy2_U8(buf0 , buf1 , width >> 1 , height >> 1 , stride >> 1);

	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf1  , buf2 , width >> 2 , height >> 2 , stride >> 2);
	BoxBlur_3x3_U8(buf2  , buf1 , width >> 2 , height >> 2 , stride >> 2);
	//组合成pyrUp
	upScaleBy2_U8(buf1 , buf2 , width >> 2 , height >> 2 , stride >> 2);
	GaussBlur_5x5_U8(buf2 , pDst , width >> 1 , height >> 1 , stride >> 1);
}








//*******************************************************************************************************************//
/*												BlurUV																 */
//*******************************************************************************************************************//

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

ErrorType BlurUV_Level_2(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride)
{
	//Use Memory Pool
	uint8_t *pBuf0U = pBuf;
	uint8_t *pBuf1U = pBuf0U + height * stride;
	uint8_t *pBuf0V = pBuf1U + height * stride;
	uint8_t *pBuf1V = pBuf0V + height * stride;

	uint32_t MemoryUsePtr = (4 * height * stride);
	xa_printf("BlurUV_Level_2 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);
	//	U
	downScaleBy2_U8(psrcU , pBuf0U , width , height , stride);
	BoxBlur_3x3_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	//组合成fcvScaleUpBy2Gaussian5x5u8_v2
	upScaleBy2_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0U , pdstU , width  , height , stride );

	//	V
	downScaleBy2_U8(psrcV , pBuf0V , width , height , stride);
	BoxBlur_3x3_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	//组合成fcvScaleUpBy2Gaussian5x5u8_v2
	upScaleBy2_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0V , pdstV , width  , height , stride );
}


ErrorType BlurUV_Level_3(uint8_t *psrcU , uint8_t *psrcV , uint8_t *pdstU , uint8_t *pdstV ,
		uint8_t *pBuf , int width , int height , int stride)
{
	//Use Memory Pool
	uint8_t *pBuf0U = pBuf;
	uint8_t *pBuf1U = pBuf0U + height * stride;
	uint8_t *pBuf0V = pBuf1U + height * stride;
	uint8_t *pBuf1V = pBuf0V + height * stride;

	uint32_t MemoryUsePtr = (4 * height * stride);
	xa_printf("BlurUV_Level_3 MemoryUsePtr is 0x%x\n" , MemoryUsePtr);
	//	U
	downScaleBy2_U8(psrcU , pBuf0U , width , height , stride);
	BoxBlur_3x3_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0U , pBuf1U , width >> 1 , height >> 1 , stride >> 1);
	//组合成fcvScaleUpBy2Gaussian5x5u8_v2
	upScaleBy2_U8(pBuf1U , pBuf0U , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0U , pdstU , width  , height , stride );

	//	V
	downScaleBy2_U8(psrcV , pBuf0V , width , height , stride);
	BoxBlur_3x3_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	BoxBlur_3x3_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0V , pBuf1V , width >> 1 , height >> 1 , stride >> 1);
	//组合成fcvScaleUpBy2Gaussian5x5u8_v2
	upScaleBy2_U8(pBuf1V , pBuf0V , width >> 1 , height >> 1 , stride >> 1);
	GaussBlur_5x5_U8(pBuf0V , pdstV , width  , height , stride );
}



