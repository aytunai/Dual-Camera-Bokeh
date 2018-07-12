/*
 * main.c
 *
 *  Created on: 2018-3-23
 *      Author: AiTong
 */

#include "stdio.h"

#if defined(__XTENSA__)
#include <stdbool.h>
#include <sys/times.h>
#include <xtensa/sim.h>
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#endif

/* Image IO utilities */
#include "typedef.h"
#include "img_utils.h"
#include "tileManager.h"

#include "BoxBlur_3x3.h"
#include "BoxBlur_5x5.h"
#include "GaussBlur_3x3.h"
#include "Separate.h"
#include "Combine.h"
#include "DownScaleBy2.h"
#include "UpScaleBy2.h"
#include "GaussBlur_5x5.h"
#include "UpDepthByAvg.h"
#include "AlphaBlend.h"
#include "BlurImp.h"

#include "Debug.h"

//这个数据量小，仿真的速度快
#define PIC_WIDTH  1440
#define PIC_HEIGHT 1080
#define PIC_PITCH  1440

//全局申请Buffer内存的大小，为了适配真实的SDK开发环境的方式
#define MEM_POOL_SIZE	(PIC_WIDTH * PIC_HEIGHT * 16)

//模拟真实的开发环境
typedef struct {
	//YUV图像的分辨率
	int width;
	int height;
	int stride;
	//(暂时只支持blurYLevel = 1,blurUVLevel1)
	int blurYLevel;		//虚化力度(1-30)
	int blurUVLevel;	//虚化力度(1-3)
	//同步标志位,传递进来初始化应该清零
	int core0Status;
	int core1Status;


	//传递进来的解析作Buffer统一管理
	uint8_t *pYUV;			//YUV的图像数据	--	外部传递进来的Buffer
	uint8_t *pDepthMap;		//GPU输出的深度图	--  外部传递进来的Buffer
	uint8_t *pOut;			//输出的虚化数据	--	外部传递进来的Buffer
	uint8_t *pMemoryPool;	//统一内存管理
	uint8_t *pMemoryPool0;	//算法中间所需要的内存池(统一内存管理,解决掉malloc memory的问题),Core0的Global Memory
	uint8_t *pMemoryPool1;	//算法中间所需要的内存池(统一内存管理,解决掉malloc memory的问题),Core1的Global Memory
	uint8_t *pMessageBuff;	//VPU调试的buffer信息(CPU端解析并输出结果)
}CPU2VPU_Para;


int main()
{
	//这些Buffer都是预先在CPU端开辟好的，从外部传递进来的
	yuv_t yuv , yuv_out;
	yuv.pYUV = (uint8_t *)malloc( (PIC_PITCH * PIC_HEIGHT) * sizeof(uint8_t) * 3 / 2);
	yuv.pY	 = (uint8_t *)yuv.pYUV;
	yuv.pUV  = (uint8_t *)yuv.pYUV + (PIC_PITCH * PIC_HEIGHT);
	yuv.pU   = (uint8_t *)malloc( (PIC_PITCH >> 1) * (PIC_HEIGHT >> 1) * sizeof(uint8_t) );
	yuv.pV   = (uint8_t *)malloc( (PIC_PITCH >> 1) * (PIC_HEIGHT >> 1) * sizeof(uint8_t) );

	yuv_out.pYUV = (uint8_t *)malloc( (PIC_PITCH * PIC_HEIGHT) * sizeof(uint8_t) * 3 / 2);
	yuv_out.pY   = (uint8_t *)yuv_out.pYUV;
	yuv_out.pUV  = (uint8_t *)yuv_out.pYUV + (PIC_PITCH * PIC_HEIGHT);
	yuv_out.pU   = (uint8_t *)malloc( (PIC_PITCH >> 1) * (PIC_HEIGHT >> 1) * sizeof(uint8_t) );
	yuv_out.pV   = (uint8_t *)malloc( (PIC_PITCH >> 1) * (PIC_HEIGHT >> 1) * sizeof(uint8_t) );
	uint8_t *pdepth = (uint8_t *)malloc(PIC_PITCH * PIC_HEIGHT / 4);

	//申请global memory pool
	uint8_t *pMemoryPool = (uint8_t *)malloc(MEM_POOL_SIZE);


#ifdef DEBUG_READ_YUV_FILE
	unsigned char YUVFileName[256];
	sprintf(YUVFileName , "%s//in_yuv_1440_1080" , INPUT_FILE_PATH);
	FILE *fpInput = fopen(YUVFileName , "rb+");
	if(fpInput == NULL){
		printf("Open YUV File Error\n");
		return 0;
	}
	fread(yuv.pYUV , (PIC_PITCH * PIC_HEIGHT) * sizeof(uint8_t) * 3 / 2 , sizeof(uint8_t) , fpInput);
	fclose(fpInput);
#endif

#ifdef DEBUG_READ_DEPTH_FILE
	unsigned char depthFileName[256];
	sprintf(depthFileName , "%s//in_alpha_720_540" , INPUT_FILE_PATH);
	FILE *fpDepth = fopen(depthFileName , "rb+");
	if(fpDepth == NULL){
		printf("Open Depth File Error\n");
		return 0;
	}
	fread(pdepth , (PIC_PITCH * PIC_HEIGHT) * sizeof(uint8_t) / 4 , sizeof(uint8_t) , fpDepth);
	fclose(fpDepth);
#endif

	int time_start , time_end;

	TIME_STAMP(time_start);

	//模拟Core-0单核的开发方式
	CPU2VPU_Para gPara , *para;
	para = &gPara;
	para->width  = PIC_WIDTH;
	para->height = PIC_HEIGHT;
	para->stride = PIC_PITCH;
	para->blurYLevel = 30;
	para->blurUVLevel = 1;
	para->pYUV = yuv.pYUV;
	para->pDepthMap = pdepth;
	para->pOut = yuv_out.pYUV;
	para->pMemoryPool0 = pMemoryPool;
	para->pMemoryPool1 = pMemoryPool;
	para->pMessageBuff = NULL;

	if(CORE_ID == 1){
		para->pMemoryPool = para->pMemoryPool0;
	}
	else{
		para->pMemoryPool = para->pMemoryPool1;
	}
	uint32_t MemStart , MemEnd;
	MemStart = (uint32_t)para->pMemoryPool;
	xa_printf("MemoryPool Start Addr:0x%x\n" , MemStart);

	//Check The Parameter
	int blurYLevel = para->blurYLevel , blurUVLevel = para->blurUVLevel;
	int width = para->width , height = para->height , stride = para->stride;
	xa_printf("blurYLevel , blurUVLevel , width, height , stride {%d,%d,%d,%d,%d}\n" ,
			blurYLevel , blurUVLevel , width , height , stride);

	//Global Memory Manager
	uint8_t *pSrcUV = para->pYUV + stride * height;
	int widthU = width >> 1 , heightU = height >> 1 , strideU = stride >> 1 ;
	int widthV = width >> 1 , heightV = height >> 1 , strideV = stride >> 1 ;

	uint8_t *pSrcU = para->pMemoryPool;		int U_Len = (heightU * strideU);
	para->pMemoryPool += XA_ALIGN(U_Len , ALIGNED_64B);
	uint8_t *pSrcV = para->pMemoryPool;		int V_Len = (heightV * strideV);
	para->pMemoryPool += XA_ALIGN(V_Len , ALIGNED_64B);

	uint8_t *pBlurY = para->pMemoryPool;	int blurY_Len = (height >> 1) * (stride >> 1);
	para->pMemoryPool += XA_ALIGN(blurY_Len, ALIGNED_64B);
	uint8_t *pBlurU = para->pMemoryPool;	int blurU_Len = (heightU * strideU);
	para->pMemoryPool += XA_ALIGN(blurU_Len, ALIGNED_64B);
	uint8_t *pBlurV = para->pMemoryPool;	int blurV_Len = (heightV * strideV);
	para->pMemoryPool += XA_ALIGN(blurV_Len, ALIGNED_64B);

	uint8_t *pUpBlurY = para->pMemoryPool;	int upBlurY_Len = (height * stride);
	para->pMemoryPool += XA_ALIGN(upBlurY_Len, ALIGNED_64B);
	uint8_t *pUpDepth = para->pMemoryPool;	int upDepth_Len = (height * stride);
	para->pMemoryPool += XA_ALIGN(upDepth_Len, ALIGNED_64B);

	uint8_t *pOutU = para->pMemoryPool;		int OutU_Len = (heightU * strideU);
	para->pMemoryPool += XA_ALIGN(OutU_Len, ALIGNED_64B);
	uint8_t *pOutV = para->pMemoryPool;		int OutV_Len = (heightV * strideV);
	para->pMemoryPool += XA_ALIGN(OutV_Len, ALIGNED_64B);

	MemEnd = (uint32_t)para->pMemoryPool;
	xa_printf("MemoryPool End Addr:0x%x\n" , MemEnd);
	xa_printf("MemoryPool Used:0x%x\n" , (MemEnd - MemStart) );
	if( (MemEnd - MemStart) > MEM_POOL_SIZE){
		xa_printf("Run Out of Memory\n");
		return 0;
	}

	for(blurYLevel = 1 ; blurYLevel <= 30; blurYLevel++){


	UV_Separate_U8(pSrcUV , pSrcU , pSrcV , widthU , heightU , strideU);
	uint8_t *pSrcY = para->pYUV;
	if( blurYLevel == 1){
		BlurY_Level_1(pSrcY , pBlurY , width , height , stride);
	}
	else if(blurYLevel == 2){
		BlurY_Level_2(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 3){
		BlurY_Level_3(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 4){
		BlurY_Level_4(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 5){
		BlurY_Level_5(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 6){
		BlurY_Level_6(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 7){
		BlurY_Level_7(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 8){
		BlurY_Level_8(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 9){
		BlurY_Level_9(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 10){
		BlurY_Level_10(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 11){
		BlurY_Level_11(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 12){
		BlurY_Level_12(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 13){
		BlurY_Level_13(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 14){
		BlurY_Level_14(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 15){
		BlurY_Level_15(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 16){
		BlurY_Level_16(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 17){
		BlurY_Level_17(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 18){
		BlurY_Level_18(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 19){
		BlurY_Level_19(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 20){
		BlurY_Level_20(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 21){
		BlurY_Level_21(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 22){
		BlurY_Level_22(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 23){
		BlurY_Level_23(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 24){
		BlurY_Level_24(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 25){
		BlurY_Level_25(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 26){
		BlurY_Level_26(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 27){
		BlurY_Level_27(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 28){
		BlurY_Level_28(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 29){
		BlurY_Level_29(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else if(blurYLevel == 30){
		BlurY_Level_30(pSrcY , pBlurY , para->pMemoryPool , width , height , stride);
	}
	else{
		xa_printf("Other BlurY Level Not Support!\n");
	}

	if(blurUVLevel == 1){
		BlurUV_Level_1(pSrcU , pSrcV , pBlurU , pBlurV , para->pMemoryPool , widthU , heightU , strideU);
	}
	if(blurUVLevel == 2){
		BlurUV_Level_2(pSrcU , pSrcV , pBlurU , pBlurV , para->pMemoryPool , widthU , heightU , strideU);
	}
	if(blurUVLevel == 3){
		BlurUV_Level_3(pSrcU , pSrcV , pBlurU , pBlurV , para->pMemoryPool , widthU , heightU , strideU);
	}
	else{
		xa_printf("Other BlurUV Level Not Support!\n");
	}

	//Alpha Blending - Y
	UpDepthByAvg_U8(pBlurY          , pUpBlurY , width >> 1 , height >> 1 , stride >> 1);
	UpDepthByAvg_U8(para->pDepthMap , pUpDepth , width >> 1 , height >> 1 , stride >> 1);
	uint8_t *pOutY = para->pOut;
	AlphaBlend_U8(pSrcY , pUpBlurY , pUpDepth , pOutY , width , height , stride);

	//Alpha Blending - UV
	AlphaBlend_U8(pSrcU , pBlurU , para->pDepthMap , pOutU , widthU , heightU , strideU);
	AlphaBlend_U8(pSrcV , pBlurV , para->pDepthMap , pOutV , widthV , heightV , strideV);
	uint8_t *pOutUV = para->pOut + stride * height;
	UV_Combine_U8(pOutUV , pOutU , pOutV , widthU , heightU , strideU);

    TIME_STAMP(time_end);


    int runtime = time_end - time_start;
    xa_printf("cycles is %d\n" , runtime);




	//单元模块测试
//	UV_Seperate_U8(yuv.pUV , yuv.pU , yuv.pV , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1) );
//
//	BoxBlur_3x3_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
//	BoxBlur_5x5_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
//	GaussBlur_3x3_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
//	downScaleBy2_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
//	upScaleBy2_U8(yuv_out.pY , yuv.pY , PIC_WIDTH >> 1 , PIC_HEIGHT >> 1 , PIC_PITCH >> 1);
//	GaussBlur_5x5_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
	//执行combine
//	UV_Combine_U8(yuv.pUV , yuv.pU , yuv.pV , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1));

//	UpDepthByAvg_U8(pdepth , pUpDepth , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1));
//	BoxBlur_5x5_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);
//	AlphaBlend_U8(yuv.pY , yuv_out.pY , pUpDepth , pBokeh , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);


#ifdef DEBUG_SEPERATE_OUTPUT_FILE
	//调试输出的Output结果
	unsigned char UFileName[256] , VFileName[256];
	sprintf(UFileName   , "%s//u_720_540" , OUTPUT_FILE_PATH);
	sprintf(VFileName   , "%s//v_720_540" , OUTPUT_FILE_PATH);
	remove(UFileName);
	remove(VFileName);
	FILE *fpU = fopen(UFileName , "wb+");
	FILE *fpV = fopen(VFileName , "wb+");
	if( (fpU == NULL) || (fpV == NULL) ){
		printf("Open U&V File Error\n");
		return 0;
	}
	fwrite(yuv.pU , (PIC_HEIGHT >> 1) * (PIC_PITCH >> 1) , sizeof(uint8_t) , fpU);
	fwrite(yuv.pV , (PIC_HEIGHT >> 1) * (PIC_PITCH >> 1) , sizeof(uint8_t) , fpV);
	fclose(fpU);
	fclose(fpV);
#endif



#ifdef DEBUG_COMBINE_OUTPUT_FILE
	unsigned char UVFileName[256];
	sprintf(UVFileName   , "%s//uv_720_540" , OUTPUT_FILE_PATH);
	remove(UVFileName);
	FILE *fpUV = fopen(UVFileName , "wb+");
	fwrite(yuv.pUV , (PIC_PITCH >> 1) * (PIC_HEIGHT >> 1) , sizeof(uint8_t) * 2 , fpUV);
	fclose(fpUV);

#endif

#ifdef DEBUG_OUTPUT_BOXBLUR_3x3_FILE
	char BlurYFileName[256];
	sprintf(BlurYFileName, "%s//blur_y_3x3_1440_1080", OUTPUT_FILE_PATH);
	remove(BlurYFileName);

	FILE *fpYUVOut = fopen(BlurYFileName , "wb+");
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_BOXBLUR_5x5_FILE
	char BlurYFileName[256];
	sprintf(BlurYFileName, "%s//blur_y_5x5_1440_1080", OUTPUT_FILE_PATH);
	remove(BlurYFileName);

	FILE *fpYUVOut = fopen(BlurYFileName , "wb+");
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_GAUSSBLUR_3x3_FILE
	char GaussBlurYFileName[256];
	sprintf(GaussBlurYFileName, "%s//gaussBlur_y_3x3_1440_1080", OUTPUT_FILE_PATH);
	remove(GaussBlurYFileName);

	FILE *fpYUVOut = fopen(GaussBlurYFileName , "wb+");
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_DOWNSCALE_BY2_FILE
	char DownScaleBy2FileName[256];
	sprintf(DownScaleBy2FileName, "%s//downScaleBy2_1440_1080", OUTPUT_FILE_PATH);
	remove(DownScaleBy2FileName);

	FILE *fpYUVOut = fopen(DownScaleBy2FileName , "wb+");
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT / 4), sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_UPSCALE_BY2_FILE
	char UpScaleBy2FileName[256];
	sprintf(UpScaleBy2FileName, "%s//upScaleBy2_1440_1080", OUTPUT_FILE_PATH);
	remove(UpScaleBy2FileName);

	FILE *fpYUVOut = fopen(UpScaleBy2FileName , "wb+");
	fwrite(yuv.pY , PIC_PITCH * PIC_HEIGHT, sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_GAUSSBLUR_5x5_FILE
	char GaussBlur5x5FileName[256];
	sprintf(GaussBlur5x5FileName, "%s//gaussBlur_y_5x5_1440_1080", OUTPUT_FILE_PATH);
	remove(GaussBlur5x5FileName);

	FILE *fpYUVOut = fopen(GaussBlur5x5FileName , "wb+");
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif

#ifdef DEBUG_OUTPUT_DEPTH_UPSCALEAVG_FILE
	char UpDepthFileName[256];
	sprintf(UpDepthFileName, "%s//out_alpha_1440_1080", OUTPUT_FILE_PATH);
	remove(UpDepthFileName);

	FILE *fpUpDepth = fopen(UpDepthFileName , "wb+");
	fwrite(pUpDepth , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpUpDepth);
	fclose(fpUpDepth);
#endif

#ifdef DEBUG_OUTPUT_Y_BOKEH_FILE
	char YBokehFileName[256];
	sprintf(YBokehFileName, "%s//y_bokeh_1440_1080", OUTPUT_FILE_PATH);
	remove(YBokehFileName);

	FILE *fpYBokeh = fopen(YBokehFileName , "wb+");
	fwrite(pBokeh , (PIC_PITCH * PIC_HEIGHT), sizeof(uint8_t) , fpYBokeh);
	fclose(fpYBokeh);
#endif

#ifdef DEBUG_OUTPUT_YUV_FILE
	char YUVOutFileName[256];
	sprintf(YUVOutFileName, "%s//yuv_out_1440_1080_%d_%d", OUTPUT_FILE_PATH , blurYLevel,blurUVLevel);
	remove(YUVOutFileName);

	FILE *fpYUVOut = fopen(YUVOutFileName , "wb+");
	fwrite(yuv_out.pYUV , (PIC_PITCH * PIC_HEIGHT) * 3 / 2 , sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVOut);
#endif
	}


	free(yuv.pYUV);
	free(yuv.pU);
	free(yuv.pV);
	free(yuv_out.pYUV);
	free(yuv_out.pU);
	free(yuv_out.pV);

	free(pdepth);
	free(pMemoryPool);

	return 0;
}
















