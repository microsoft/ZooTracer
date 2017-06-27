#pragma once

#include <memory>
#include <vector>

namespace zt{

	class ImmutableBitmap;
	using Image = std::shared_ptr<ImmutableBitmap>;

	///<summary>Immutable image</summary>
	class ImmutableBitmap{
	public:
		virtual unsigned char const * data(int row=0) const = 0;

		// Returns the image pixel size in bytes.
		virtual size_t pixel_size() const = 0;

		// Returns the number of color channels.
		virtual int num_channels() const = 0;

		// Returns the number of raster columns.
		virtual int width() const = 0;

		// Returns the number of raster rows.
		virtual int height() const = 0;

		// Returns the number of bytes between image rows.
		virtual int stride() const = 0;

		// Returns total number of pixels in the image.
		virtual size_t num_pixels() const = 0;

		// Returns a copy of image data.
		virtual std::vector<unsigned char> image_data() const = 0;

		// Creates a new image with reference to a rectangle in this image, possibly without copying the image data.
		virtual Image subImage(int x1, int y1, int width, int height) const = 0;

	};

	Image make_image(int width, int height, int pixel_size, const std::vector<unsigned char>& data);
}

