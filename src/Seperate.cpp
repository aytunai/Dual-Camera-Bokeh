
#include "Seperate.h"

int seperateUV(const cv::Mat& UV, const cv::Mat& U, const cv::Mat& V)
{
	int rows = UV.rows;
	int cols = UV.cols / 2;

	uint8_t *puv = UV.data;
	uint8_t *pu  = U.data;
	uint8_t *pv  = V.data;

	int i , j;
	for (j = 0; j < rows; j++){
		for (i = 0; i < cols; i++){
			*pu++ = *puv++;
			*pv++ = *puv++;
		}
	}

	return 0;
}




