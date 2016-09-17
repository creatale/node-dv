#ifndef BINARIZEADAPTIVE
#define BINARIZEADAPTIVE

#include "opencv2/core/core.hpp"

cv::Mat binarizeNiblack (cv::Mat input, int winx, int winy, float optK);
cv::Mat binarizeSauvola (cv::Mat input, int winx, int winy, float optK);
cv::Mat binarizeWolf (cv::Mat input, int winx, int winy, float optK);
cv::Mat binarizeNick (cv::Mat input, int winx, int winy, float optK);

#endif
