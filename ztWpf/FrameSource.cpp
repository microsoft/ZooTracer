#include "stdafx.h"

#include "FrameSource.h"

#include <ztKDTree.h>

#include <msclr\marshal_cppstd.h>
#include <sstream>

using System::String;
using System::IntPtr;
using System::Tuple;
using System::Collections::Generic::IList;
using System::Collections::Generic::List;
using System::Windows::Media::PixelFormats;
using System::Windows::Media::Imaging::WriteableBitmap;
using System::Windows::Media::Imaging::BitmapSource;
using System::Windows::Media::PixelFormats;
using System::Windows::Media::ImageSource;
using System::Windows::Int32Rect;

using std::vector;
using namespace ztWpf;
using namespace msclr::interop;

FrameSource::FrameSource(String^ fileName)
: video{ *(new OpenCVFrameSource(marshal_as<std::string>(fileName))) }
{}

FrameSource::~FrameSource(){
	delete &video;
}


System::Windows::Size FrameSource::getFrameSize(){
	return System::Windows::Size(video.frameWidth(), video.frameHeight());
}

int FrameSource::numFrames::get(){ return video.numFrames(); }
int FrameSource::frameWidth::get(){ return video.frameWidth(); }
int FrameSource::frameHeight::get(){ return video.frameHeight(); }
double FrameSource::framesPerSecond::get(){ return video.framesPerSecond(); }
String^ FrameSource::fourCC::get(){ return marshal_as<String^>(video.fourCC()); }

WriteableBitmap^ FrameSource::getBitmap(){
	// Format24bppRgb Specifies that the format is 24 bits per pixel; 8 bits each are used for the red, green, and blue components
	auto pf = PixelFormats::Bgr24;
	return gcnew WriteableBitmap(video.frameWidth(), video.frameHeight(), 96.0, 96.0, pf, nullptr);
}

bool FrameSource::writeFrame(WriteableBitmap^ bm, int frameNo){
	if (frameNo >= video.numFrames()) return false;
	Image frame;
	try {
		frame = video.getFrame(frameNo);
	}
	catch (std::exception e){
		throw gcnew System::Exception(marshal_as<String^>(e.what()));
	}
	if (frame->width() != bm->PixelWidth
		|| frame->height() != bm->PixelHeight
		|| frame->num_channels() != 3
		|| frame->pixel_size() != 3) return false;
	bm->WritePixels(
		System::Windows::Int32Rect(0, 0, bm->PixelWidth, bm->PixelHeight),
		IntPtr((void*)frame->data()),
		frame->num_pixels()*frame->pixel_size(),
		frame->stride(), 
		0, 0);
	return true;
}

// Writes a sub-image at the supplied offset. Actual offset may be different;
System::Tuple<int,int>^ FrameSource::writeFrame(WriteableBitmap^ bm, int frameNo, int x, int y){
	if (frameNo > video.numFrames()) return Tuple::Create(-1,-1);
	auto frame = video.getFrame(frameNo);
	if (frame->width() < bm->PixelWidth
		|| frame->height() < bm->PixelHeight
		|| frame->num_channels() != 3
		|| frame->pixel_size() != 3) return Tuple::Create(-1, -1);
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x + bm->PixelWidth>frame->width()) x = frame->width() - bm->PixelWidth;
	if (y + bm->PixelHeight>frame->height()) y = frame->height() - bm->PixelHeight;
	Image patch = frame->subImage(x, y, bm->PixelWidth, bm->PixelHeight);
	bm->WritePixels(
		System::Windows::Int32Rect(0, 0, bm->PixelWidth, bm->PixelHeight),
		IntPtr((void*)patch->data()),
		patch->stride()*patch->height(),
		patch->stride(),
		0, 0);
	return Tuple::Create(x,y);
}

ImageSource^ FrameSource::getPatch(int frameNo, Int32Rect^ patch){
	Image frame = video.getFrame(frameNo);
	Image p = frame->subImage(patch->X, patch->Y, patch->Width, patch->Height);
	return BitmapSource::Create(p->width(), p->height(), 96, 96, PixelFormats::Bgr24, nullptr,
		IntPtr((void*)p->data()), p->stride()*p->height(), p->stride());
}