
#include "Combine.h"

int combineUV(const cv::Mat& U, const cv::Mat& V, const cv::Mat& UV)
{
	int rows = U.rows;
	int cols = U.cols;

	uint8_t *puv = UV.data;
	uint8_t *pu = U.data;
	uint8_t *pv = V.data;

	int i, j;
	for (j = 0; j < rows; j++){
		for (i = 0; i < cols; i++){
			*puv++ = *pu++;
			*puv++ = *pv++;
		}
	}

	return 0;
}

