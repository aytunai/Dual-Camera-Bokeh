
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

