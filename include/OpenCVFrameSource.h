#pragma once

#include "ztImage.h"
#include <string>

namespace zt{

	struct size{
		short width;
		short height;
	};

	class OpenCVFrameSource{
	public:
		OpenCVFrameSource(std::string);
		~OpenCVFrameSource();
		int frameWidth() const;
		int frameHeight() const;
		int numFrames() const;
		double framesPerSecond() const;
		std::string fourCC() const;
		Image getFrame(int frameNo) const;
	private:
		class implementation; std::unique_ptr<implementation> impl;
	};
}

