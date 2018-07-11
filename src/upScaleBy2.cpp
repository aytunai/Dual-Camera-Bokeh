
#include "upScaleBy2.h"

int upScaleBy2(const cv::Mat& src, const cv::Mat& dst)
{
	int cols = src.cols;
	int rows = src.rows;

	uint8_t *psrc = src.data;
	uint8_t *pdst = dst.data;

	int spitch = cols;
	int dpitch = 2 * cols;

	int i, j;
	for (j = 0; j < rows; j++, psrc += spitch , pdst += 2 * dpitch /*dst需要跳两行*/	){

		for (i = 0; i < cols; i++){

			int cp = psrc[i];

			pdst[2 * i] = cp;
			pdst[2 * i + 1] = cp;
			pdst[2 * i + dpitch] = cp;
			pdst[2 * i + dpitch + 1] = cp;
		}
	}

	return 0;
}

/*
**	先左右插值,取均值.然后再上下插值
*/
int upScaleByAvg2(const cv::Mat& src, const cv::Mat& dst)
{
	int cols = dst.cols;
	int rows = dst.rows;

	uint8_t *psrc = src.data;
	uint8_t *pdst = dst.data;


	int i, j;
	//先处理偶数行，再处理奇数行
	for (j = 0; j < rows; j += 2){
		for (i = 0; i < cols; i += 2){

			uint8_t curPixels = psrc[(j / 2) * (cols / 2) + (i / 2)];
			uint8_t nextPixels;
			//边界处理
			if (i == (cols - 2)){
				nextPixels = curPixels;
			}
			else{
				nextPixels = psrc[(j / 2) * (cols / 2) + (i / 2) + 1];
			}

			uint8_t avg = ((uint32_t)curPixels + (uint32_t)nextPixels + 1) / 2;
			pdst[j * cols + i] = curPixels;
			pdst[j * cols + i + 1] = avg;

		}
	}

	//处理奇数行
	for (j = 1; j < rows; j += 2){
		for (i = 0; i < cols; i ++){

			uint8_t topPixels = pdst[(j - 1) * cols + i];
			uint8_t botPixels;
			//边界处理
			if (j == (rows - 1)){
				botPixels = topPixels;
			}
			else{
				botPixels = pdst[(j + 1) * cols + i];
			}

			uint8_t avg = ((uint32_t)topPixels + (uint32_t)botPixels + 1) / 2;
			pdst[j * cols + i] = avg;

		}
	}

	return 0;
}

