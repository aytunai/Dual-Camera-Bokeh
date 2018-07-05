
#include "OpencvCommon.h"
#include "Seperate.h"
#include "Combine.h"
#include "downScaleBy2.h"
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

	uint8_t *pY  = pYUV;
	uint8_t *pUV = pY + (IMG_PITCH * IMG_HEIGHT);

	Mat  yImg(IMG_HEIGHT	 , IMG_PITCH     , CV_8U, pY);
	Mat uvImg(IMG_HEIGHT >> 1, IMG_PITCH     , CV_8U, pUV);
	Mat  uImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  vImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);
	Mat  downScaleImg(IMG_HEIGHT >> 1, IMG_PITCH >> 1, CV_8U);

	Mat yOutImg(IMG_HEIGHT, IMG_PITCH, CV_8U);

	downScaleBy2(yImg, downScaleImg);

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

#if 0
	char GaussBlurY5x5FileName[FILENAME_LEN];
	sprintf(GaussBlurY5x5FileName, "%s//gaussBlur_y_3x3_1440_1080", OUTPUT_FILE_PATH);
	remove(GaussBlurY5x5FileName);

	ofstream gaussBlurY5x5File;
	gaussBlurY5x5File.open(GaussBlurY5x5FileName, ios::binary);
	if (!gaussBlurY5x5File){
		printf("read UV File Error\n");
		return 0;
	}
	gaussBlurY5x5File.write((char *)yOutImg.data, IMG_HEIGHT * IMG_PITCH);

#endif

#if 1
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

#ifdef PC_TEST
	imshow("yImg", yImg);
	imshow("yOutImg", yOutImg);
	imshow("downScaleImg", downScaleImg);

	cvWaitKey(0);
#endif

	return NO_ERROR;
}
