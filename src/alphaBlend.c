

#include "alphaBlend.h"

int alphaBlend(const cv::Mat& InA, const cv::Mat& InB, const cv::Mat& Alpha, const cv::Mat& dst)
{
	int cols = dst.cols;
	int rows = dst.rows;

	uint8_t *pInA   = InA.data;
	uint8_t *pInB   = InB.data;
	uint8_t *pAlpha = Alpha.data;
	uint8_t *pOut   = dst.data;

	int i, j;
	for (j = 0; j < rows; j++){
		for (i = 0; i < cols; i++){
			int idx = j * cols + i;

			uint32_t t1 = (uint32_t)pInA[idx] * (uint32_t)pAlpha[idx];
			uint32_t t2 = (uint32_t)pInB[idx] * (255 - pAlpha[idx]);

			pOut[idx] = (t1 + t2 + 128) / 256;		//rounding
		}
	}


	return 0;
}