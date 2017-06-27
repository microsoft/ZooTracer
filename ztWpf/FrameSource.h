// ztWpf.h

#pragma once
#include <OpenCVFrameSource.h>

namespace ztWpf{
	using zt::OpenCVFrameSource;
	using zt::Image;
	using System::Windows::Media::Imaging::WriteableBitmap;
	using System::Windows::Media::ImageSource;
	using System::Windows::Int32Rect;

	public ref class FrameSource
	{
	private:
		OpenCVFrameSource& video;
	public:
		FrameSource(System::String^ fileName);
		~FrameSource();
		OpenCVFrameSource& GetFrameSource(){ return video; }

		System::Windows::Size getFrameSize();
		property int numFrames {int get(); }
		property int frameWidth{int get(); }
		property int frameHeight{int get(); }
		property double framesPerSecond{double get(); }
		property System::String^ fourCC{System::String^ get(); }
		WriteableBitmap^ getBitmap();
		bool writeFrame(WriteableBitmap^ bm, int frameNo);

		// Writes a sub-image at the supplied offset. Actual offset may be different.
		System::Tuple<int, int>^ writeFrame(WriteableBitmap^ bm, int frameNo, int x, int y);

		ImageSource^ getPatch(int frameNo, Int32Rect^ patch);

		Image getFrame(int frameNo){ return video.getFrame(frameNo); }
	};

}
