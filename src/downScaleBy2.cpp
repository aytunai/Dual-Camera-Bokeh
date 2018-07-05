
#include "downScaleBy2.h"


int downScaleBy2(const cv::Mat& src, const cv::Mat& dst)
{
	int cols = dst.cols;
	int rows = dst.rows;

	uint8_t *psrc = src.data;
	uint8_t *pdst = dst.data;

	int spitch = 2 * cols;
	int dpitch = cols;

	int i, j;
	for (j = 0; j < rows; j++, pdst += dpitch, psrc += spitch * 2){

		for (i = 0; i < cols; i++){

			int a = psrc[2 * i];
			int b = psrc[2 * i + 1];
			int c = psrc[2 * i + spitch];
			int d = psrc[2 * i + 1 + spitch];
			int result = a + b + c + d;

			result = result >> 2;
			pdst[i] = (uint8_t)result;

		}

	}





	return 0;
}

