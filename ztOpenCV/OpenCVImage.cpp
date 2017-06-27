#include "OpenCVImage.h"

zt::Image zt::make_image(int width, int height, int pixel_size, const std::vector<uchar>& data){
	// copyData = true
	return std::make_shared<OpenCVImage>(cv::Mat { data, true } .reshape(pixel_size, height));
}