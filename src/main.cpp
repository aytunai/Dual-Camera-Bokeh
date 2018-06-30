
#include "OpencvCommon.h"
#include "Seperate.h"
#include "common.h"

//#define INPUT_FILE_PATH	 "D://VS2013 Project//Dual_Camera_Bokeh_Preview//data//input"
//#define OUTPUT_FILE_PATH "D://VS2013 Project//Dual_Camera_Bokeh_Preview//data//output"
//路径是相对于sln的path，可能会需要进行调整
#define INPUT_FILE_PATH	 "..//data//input"
#define OUTPUT_FILE_PATH "..//data//output"

#define IMG_WIDTH			1440
#define IMG_HEIGHT			1080
#define IMG_PITCH			1440

#define FILENAME_LEN		256



int main()
{
	char inputImgFileName[FILENAME_LEN];
	sprintf(inputImgFileName, "%s//in_yuv_1440_1080", INPUT_FILE_PATH);

	uint8_t *pYUV = (uint8_t *)malloc(IMG_PITCH * IMG_HEIGHT * 3 / 2);
	FILE *fpYUV = fopen(inputImgFileName , "rb+");
	if (fpYUV == NULL){
		printf("File = %s , Line = %d , Func=%s:",__FILE__, __LINE__, __FUNCTION__);
		printf("Read Input File Error\n");
		return FILE_READ_ERROR;
	}
	fread(pYUV, IMG_PITCH * IMG_HEIGHT * 3 / 2, sizeof(uint8_t), fpYUV);
	fclose(fpYUV);


	uint8_t *pY = pYUV;
	uint8_t *pUV = pY + (IMG_PITCH * IMG_HEIGHT);


	Mat  yImg(IMG_HEIGHT	 , IMG_PITCH     , CV_8U, pY);
	Mat uvImg(IMG_HEIGHT >> 1, IMG_PITCH     , CV_8U, pUV);
	Mat  uImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  vImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

	seperateUV(uvImg, uImg, vImg);

#ifdef PC_TEST
	imshow("yImg", yImg);
	imshow("uImg", uImg);
	imshow("vImg", vImg);
	cvWaitKey(0);
#endif

	return NO_ERROR;
}
