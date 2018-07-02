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
#include "Seperate.h"
#include "Combine.h"

#include "Debug.h"

//这个数据量小，仿真的速度快
//#define PIC_WIDTH  1440
#define PIC_WIDTH  1440
#define PIC_HEIGHT 1080
#define PIC_PITCH  1440

int main()
{
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

	//执行seperate的操作
	UV_Seperate_U8(yuv.pUV , yuv.pU , yuv.pV , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1) );
#if 0
	BoxBlur_3x3_U8(yuv.pY , yuv_out.pY , PIC_WIDTH , PIC_HEIGHT , PIC_PITCH);

	BoxBlur_3x3_U8(tudata , udata , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1) );

	BoxBlur_3x3_U8(tvdata , vdata , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1) );
#endif
	//执行combine
	UV_Combine_U8(yuv.pUV , yuv.pU , yuv.pV , (PIC_WIDTH >> 1) , (PIC_HEIGHT >> 1) , (PIC_PITCH >> 1));


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
	remove("C://dump//boxBlur3x3//cadence//yuvIn.bin");
	remove("C://dump//boxBlur3x3//cadence//yuvOut.bin");
	FILE *fpYUVIn  = fopen("C://dump//boxBlur3x3//yuvIn.bin" , "wb+");
	FILE *fpYUVOut = fopen("C://dump//boxBlur3x3//yuvOut.bin" , "wb+");
	fwrite(yuv.pY     , (PIC_PITCH * PIC_HEIGHT) + ((PIC_PITCH * PIC_HEIGHT) >> 1) , sizeof(uint8_t) , fpYUVIn);
	fwrite(yuv_out.pY , (PIC_PITCH * PIC_HEIGHT) + ((PIC_PITCH * PIC_HEIGHT) >> 1) , sizeof(uint8_t) , fpYUVOut);
	fclose(fpYUVIn);
	fclose(fpYUVOut);
#endif


	free(yuv.pYUV);
	free(yuv.pU);
	free(yuv.pV);
	free(yuv_out.pYUV);
	free(yuv_out.pU);
	free(yuv_out.pV);

	return 0;
}
















