#pragma once

#include <ztImage.h>
#include <memory>
#include <opencv2/core/core.hpp>
#include <cassert>

namespace zt{

	class OpenCVImage : public ImmutableBitmap, private cv::Mat
	{
	public:
		OpenCVImage(const cv::Mat& bitmap) : cv::Mat(bitmap){
			if (bitmap.dims != 2) throw std::exception("invalid bitmap");
		}
		~OpenCVImage(){}
		uchar const * data(int row) const override { return cv::Mat::ptr(row); }

		// Returns the image pixel size in bytes.
		size_t pixel_size() const override { return cv::Mat::elemSize(); }

		// Returns the number of color channels.
		int num_channels() const override { return cv::Mat::channels(); }

		// Returns the number of raster columns.
		int width() const override{ return cv::Mat::cols; }

		// Returns the number of raster rows.
		int height() const override{ return cv::Mat::rows; }

		// Returns the number of bytes between image rows.
		int stride() const override{ return static_cast<int>(cv::Mat::ptr(1) - cv::Mat::ptr(0)); }

		// Returns total number of pixels in the image
		size_t num_pixels() const override{ return cv::Mat::total(); }

		// Returns a copy of image data.
		std::vector<uchar> image_data() const override{
			int row_size = width()*pixel_size();
			int rows = height();
			std::vector<uchar> result(rows*row_size);
			uchar* d = result.data();
			if (isContinuous()){
				memcpy(d, cv::Mat::ptr(0), rows*row_size);
			}
			else{
				for (int r = 0; r < rows; ++r){
					memcpy(d, cv::Mat::ptr(r), row_size);
					d += row_size;
				}
			}
			return result;
		}

		// Creates a new image with reference to a rectangle in this image, possibly without copying the image data.
		Image subImage(int x, int y, int width, int height) const override {
			return std::make_shared<OpenCVImage>((*this)(cv::Rect(x, y, width, height)));
		}

	};

}