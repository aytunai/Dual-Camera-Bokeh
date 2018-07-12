
#include "OpencvCommon.h"
#include "Seperate.h"
#include "Combine.h"
#include "downScaleBy2.h"
#include "upScaleBy2.h"
#include "alphaBlend.h"
#include "common.h"
#include "iostream"
#include "fstream"
using namespace std;

#define INPUT_FILE_PATH	 "D://UnitTest//dump//Bokeh_preview//PC//input"
#define OUTPUT_FILE_PATH "D://UnitTest//dump//Bokeh_preview//PC//output"
//路径是相对于sln的path，可能会需要进行调整
//#define INPUT_FILE_PATH	 "..//data//input"
//#define OUTPUT_FILE_PATH "..//data//output"

#define IMG_WIDTH			1440
#define IMG_HEIGHT			1080
#define IMG_PITCH			1440
#define PIC_WIDTH			IMG_WIDTH
#define PIC_HEIGHT			IMG_HEIGHT
#define PIC_PITCH			IMG_PITCH

#define FILENAME_LEN		256

int main()
{
	char inputImgFileName[FILENAME_LEN], depthFileName[FILENAME_LEN];
	sprintf(inputImgFileName, "%s//in_yuv_1440_1080", INPUT_FILE_PATH);
	sprintf(depthFileName, "%s//in_alpha_720_540", INPUT_FILE_PATH);
	//YUV图像的原始数据
	uint8_t *pYUV = (uint8_t *)malloc(IMG_PITCH * IMG_HEIGHT * 3 / 2);
	FILE *fpYUV = fopen(inputImgFileName , "rb+");
	if (fpYUV == NULL){
		printf("File = %s , Line = %d , Func=%s:",__FILE__, __LINE__, __FUNCTION__);
		printf("Read Input File Error\n");
		return FILE_READ_ERROR;
	}
	fread(pYUV, IMG_PITCH * IMG_HEIGHT * 3 / 2, sizeof(uint8_t), fpYUV);
	fclose(fpYUV);
	//Depth图数据
	uint8_t *pDepth = (uint8_t *)malloc(IMG_PITCH * IMG_HEIGHT / 4);
	FILE *fpDepth = fopen(depthFileName , "rb+");
	if (fpDepth == NULL){
		printf("File = %s , Line = %d , Func=%s:", __FILE__, __LINE__, __FUNCTION__);
		printf("Read Depth File Error\n");
		return FILE_READ_ERROR;
	}
	fread(pDepth, IMG_PITCH * IMG_HEIGHT / 4, sizeof(uint8_t), fpDepth);
	fclose(fpDepth);
	//输出的YUV结果
	uint8_t *pYUVOut = (uint8_t *)malloc(IMG_PITCH * IMG_HEIGHT * 3 / 2);

	uint8_t *pY     = pYUV;
	uint8_t *pUV    = pY + (IMG_PITCH * IMG_HEIGHT);
	uint8_t *pYOut  = pYUVOut;
	uint8_t *pUVOut = pYUVOut + (IMG_PITCH * IMG_HEIGHT);

	CPU2VPU_Para gPara, *para;
	para = &gPara;
	para->width = PIC_WIDTH;
	para->height = PIC_HEIGHT;
	para->stride = PIC_PITCH;
	para->blurYLevel = 30;
	para->blurUVLevel = 1;

	int blurYLevel = para->blurYLevel, blurUVLevel = para->blurUVLevel;
	int width = para->width, height = para->height, stride = para->stride;
	xa_printf("blurYLevel , blurUVLevel , width, height , stride {%d,%d,%d,%d,%d}\n",
		blurYLevel, blurUVLevel, width, height, stride);

	Mat  yImg(IMG_HEIGHT	 , IMG_PITCH     , CV_8U, pY);
	Mat uvImg(IMG_HEIGHT >> 1, IMG_PITCH     , CV_8U, pUV);
	Mat  uImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  vImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

	Mat  blurYImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  blurUImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  blurVImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

	Mat  depthImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U, pDepth);
	Mat  upDepthImg(IMG_HEIGHT, IMG_PITCH, CV_8U);
	Mat  upblurYImg(IMG_HEIGHT, IMG_PITCH, CV_8U);
	

	Mat  yOutImg(IMG_HEIGHT, IMG_PITCH, CV_8U, pYOut);
	Mat uvOutImg(IMG_HEIGHT >> 1, IMG_PITCH, CV_8U, pUVOut);
	Mat  uOutImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  vOutImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

	seperateUV(uvImg, uImg, vImg);
	if (blurYLevel == 1){
		downScaleBy2(yImg, blurYImg);
	}
	else if (blurYLevel == 2){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 3){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, blurYImg, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
	}
	else if (blurYLevel == 3){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, blurYImg, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
	}
	else if (blurYLevel == 4){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 5){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 6){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 7){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		blur(buf1, blurYImg, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
	}
	else if (blurYLevel == 8){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 9){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 10){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 11){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 12){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 13){
		Mat  buf1(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		downScaleBy2(yImg, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf1, blurYImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 14){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 15){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 16){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 17){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 18){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 19){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 20){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 21){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);

		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 22){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 23){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);

		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 24){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);

		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 25){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf2, buf1, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);

		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 26){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 27){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 28){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		GaussianBlur(buf1, buf2, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);

		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 29){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf2, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else if (blurYLevel == 30){
		Mat  buf0(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  buf1(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		Mat  buf2(IMG_HEIGHT >> 2, IMG_PITCH >> 2, CV_8U);
		downScaleBy2(yImg, buf0);
		downScaleBy2(buf0, buf1);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf1, buf2, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(buf2, buf1, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);

		upScaleBy2(buf1, buf0);
		GaussianBlur(buf0, blurYImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	}
	else{
		xa_printf("Other BlurY Level Not Support!\n");
	}

	if (blurUVLevel == 1){
		Mat  blurUTmp(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
		Mat  blurVTmp(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

		blur(uImg, blurUTmp, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(blurUTmp, blurUImg, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(vImg, blurVTmp, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
		blur(blurVTmp, blurVImg, Size(3, 3), Point(-1, -1), BORDER_REPLICATE);
	}
	else{
		xa_printf("Other BlurUV Level Not Support!\n");
	}

	upScaleByAvg2(blurYImg, upblurYImg);
	upScaleByAvg2(depthImg, upDepthImg);
	alphaBlend(yImg, upblurYImg, upDepthImg, yOutImg);

	alphaBlend(uImg, blurUImg, depthImg, uOutImg);
	alphaBlend(vImg, blurVImg, depthImg, vOutImg);
	combineUV(uOutImg, vOutImg, uvOutImg);

	//upScaleByAvg2(depthImg, upDepthImg);
	//blur(yImg, yOutImg, Size(5, 5), Point(-1, -1), BORDER_REPLICATE);
	//alphaBlend(yImg, yOutImg, upDepthImg, yBokehImg);
	
	//downScaleBy2(yImg, downScaleImg);
	//upScaleBy2(downScaleImg, upScaleImg);
	//GaussianBlur(upScaleImg, upScaleGaussImg, Size(5, 5), 0.0, 0.0, BORDER_REPLICATE);
	//pyrUp(downScaleImg, compareGaussImg, Size(IMG_PITCH ,IMG_HEIGHT));

	//seperateUV(uvImg, uImg, vImg);
	//blur(yImg , yOutImg , Size(5, 5) , Point(-1,-1),  BORDER_REPLICATE);
	//GaussianBlur(yImg, yOutImg, Size(3, 3), 0.0, 0.0, BORDER_REPLICATE);
	//combineUV(uImg, vImg, uvImg);

//Seperate
#if 0
	char UFileName[256], VFileName[256];
	sprintf(UFileName, "%s//u_720_540", OUTPUT_FILE_PATH);
	sprintf(VFileName, "%s//v_720_540", OUTPUT_FILE_PATH);
	remove(UFileName);
	remove(VFileName);

	ofstream uFile, vFile;
	uFile.open(UFileName , ios::binary);
	vFile.open(VFileName, ios::binary);
	if ((!uFile) || (!vFile)){
		printf("read U&V File Error\n");
		return 0;
	}
	uFile.write((char *)uImg.data, (IMG_HEIGHT >> 1) * (IMG_PITCH >> 1));
	vFile.write((char *)vImg.data, (IMG_HEIGHT >> 1) * (IMG_PITCH >> 1));
#endif

//Combine
#if 0
	char UVFileName[256];
	sprintf(UVFileName, "%s//uv_720_540", OUTPUT_FILE_PATH);
	remove(UVFileName);

	ofstream uvFile;
	uvFile.open(UVFileName, ios::binary);
	if (!uvFile){
		printf("read UV File Error\n");
		return 0;
	}
	uvFile.write((char *)uvImg.data, (IMG_HEIGHT >> 1) * (IMG_PITCH >> 1) * 2);

#endif

//BoxBlur 3x3
#if 0
	char BlurY3x3FileName[FILENAME_LEN];
	sprintf(BlurY3x3FileName, "%s//blur_y_3x3_1440_1080", OUTPUT_FILE_PATH);
	remove(BlurY3x3FileName);

	ofstream blurY3x3File;
	blurY3x3File.open(BlurY3x3FileName, ios::binary);
	if (!blurY3x3File){
		printf("read UV File Error\n");
		return 0;
	}
	blurY3x3File.write((char *)yOutImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

//BoxBlur 5x5
#if 0
	char BlurY5x5FileName[FILENAME_LEN];
	sprintf(BlurY5x5FileName, "%s//blur_y_5x5_1440_1080", OUTPUT_FILE_PATH);
	remove(BlurY5x5FileName);

	ofstream blurY5x5File;
	blurY5x5File.open(BlurY5x5FileName, ios::binary);
	if (!blurY5x5File){
		printf("read UV File Error\n");
		return 0;
	}
	blurY5x5File.write((char *)yOutImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

//GaussBlur 3x3
#if 0
	char GaussBlurY3x3FileName[FILENAME_LEN];
	sprintf(GaussBlurY3x3FileName, "%s//gaussBlur_y_3x3_1440_1080", OUTPUT_FILE_PATH);
	remove(GaussBlurY3x3FileName);

	ofstream gaussBlurY3x3File;
	gaussBlurY3x3File.open(GaussBlurY3x3FileName, ios::binary);
	if (!gaussBlurY3x3File){
		printf("read UV File Error\n");
		return 0;
	}
	gaussBlurY3x3File.write((char *)yOutImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

//downScaleBy2
#if 0
	char DownScaleBy2FileName[FILENAME_LEN];
	sprintf(DownScaleBy2FileName, "%s//downScaleBy2_1440_1080", OUTPUT_FILE_PATH);
	remove(DownScaleBy2FileName);

	ofstream downScaleBy2File;
	downScaleBy2File.open(DownScaleBy2FileName, ios::binary);
	if (!downScaleBy2File){
		printf("read UV File Error\n");
		return 0;
	}
	downScaleBy2File.write((char *)downScaleImg.data, IMG_HEIGHT * IMG_PITCH / 4);
#endif

//upScaleBy2
#if 0
	char UpScaleBy2FileName[FILENAME_LEN];
	sprintf(UpScaleBy2FileName, "%s//upScaleBy2_1440_1080", OUTPUT_FILE_PATH);
	remove(UpScaleBy2FileName);

	ofstream upScaleBy2File;
	upScaleBy2File.open(UpScaleBy2FileName, ios::binary);
	if (!upScaleBy2File){
		printf("read UV File Error\n");
		return 0;
	}
	upScaleBy2File.write((char *)upScaleImg.data, IMG_HEIGHT * IMG_PITCH);
#endif

//GaussBlur 5x5
#if 0
	char GaussBlurY5x5FileName[FILENAME_LEN];
	sprintf(GaussBlurY5x5FileName, "%s//gaussBlur_y_5x5_1440_1080", OUTPUT_FILE_PATH);
	remove(GaussBlurY5x5FileName);

	ofstream gaussBlurY5x5File;
	gaussBlurY5x5File.open(GaussBlurY5x5FileName, ios::binary);
	if (!gaussBlurY5x5File){
		printf("read UV File Error\n");
		return 0;
	}
	gaussBlurY5x5File.write((char *)upScaleGaussImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

//upScaleyAvg2 width depth
#if 0
	char upDepthAvg2FileName[FILENAME_LEN];
	sprintf(upDepthAvg2FileName, "%s//out_alpha_1440_1080", OUTPUT_FILE_PATH);
	remove(upDepthAvg2FileName);

	ofstream upDepthFile;
	upDepthFile.open(upDepthAvg2FileName, ios::binary);
	if (!upDepthFile){
		printf("read UV File Error\n");
		return 0;
	}
	upDepthFile.write((char *)upDepthImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

//Last Bokeh Effect
#if 0
	char bokehFileName[FILENAME_LEN];
	sprintf(bokehFileName, "%s//y_bokeh_1440_1080", OUTPUT_FILE_PATH);
	remove(bokehFileName);

	ofstream bokehFile;
	bokehFile.open(bokehFileName, ios::binary);
	if (!bokehFile){
		printf("read bokeh File Error\n");
		return 0;
	}
	bokehFile.write((char *)yBokehImg.data, IMG_HEIGHT * IMG_PITCH);

#endif


	//Output YUV Data
#if 1
	char YUVOutFileName[FILENAME_LEN];
	sprintf(YUVOutFileName, "%s//yuv_out_1440_1080_%d_%d", OUTPUT_FILE_PATH, blurYLevel, blurUVLevel);
	remove(YUVOutFileName);

	ofstream yuvOutFile;
	yuvOutFile.open(YUVOutFileName, ios::binary);
	if (!yuvOutFile){
		printf("Open YUV Out File Error\n");
		return 0;
	}
	yuvOutFile.write((char *)pYUVOut, IMG_HEIGHT * IMG_PITCH * 3 / 2);

#endif
	

#ifdef PC_TEST
	//imshow("yImg", yImg);
	//imshow("yOutImg", yOutImg);
	//imshow("downScaleImg", downScaleImg);
	//imshow("upScaleImg", upScaleImg);
	//imshow("upScaleGaussImg", upScaleGaussImg);
	//imshow("compareGaussImg", compareGaussImg);

	//imshow("depthImg", depthImg);
	//imshow("upDepthImg", upDepthImg);

	cvWaitKey(0);
#endif

	free(pYUV);
	free(pDepth);
	free(pYUVOut);

	return NO_ERROR;
}
