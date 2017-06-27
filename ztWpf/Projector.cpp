#include "Stdafx.h"
#include "Projector.h"

#include <msclr\marshal_cppstd.h>
#include <vector>
#include <cassert>

using namespace ztWpf;
using namespace msclr::interop;
using uchar = unsigned char;
using std::vector;
using System::Windows::Media::Imaging::BitmapSource;
using System::Windows::Media::PixelFormats;


Projector::Projector(System::String^ fileName){
	proj = new zt::Projector(marshal_as<std::string>(fileName));
}

Image getRandomPatch(Image img, int patch_size)
{
	int x = rand() % (img->width() - patch_size);
	int y = rand() % (img->height() - patch_size);
	return img->subImage(x, y, patch_size, patch_size);
}

Projector::Projector(
	FrameSource^ vh,
	int outputDim,
	int patchSize,
	int numberOfSamples,
	System::String^ fileName,
	System::Action<System::String^>^ logger)
{
	int startFrame = 1;
	int endFrame = vh->numFrames;
	int nframesNeeded = int(sqrt(float(numberOfSamples)));
	int nStepBetweenFrames = max(1, (1 + endFrame - startFrame) / nframesNeeded);

	// calculate number of samples to take from each frame.
	int nActualFrames = 1 + (endFrame - startFrame) / nStepBetweenFrames;
	int nSamplesPerFrame = numberOfSamples / nActualFrames;
	int nSamplesTaken = 0;
	vector<Image> td;
	for (int iFrame = startFrame; iFrame <= endFrame; iFrame += nStepBetweenFrames)
	{
		try{
			Image img = vh->getFrame(iFrame - 1);
			bool lastFrame = iFrame + nStepBetweenFrames > endFrame;

			int nSamplesThisFrame = lastFrame ? numberOfSamples - nSamplesTaken : nSamplesPerFrame;
			for (int nPatch = 1; nPatch <= nSamplesThisFrame; ++nPatch)
			{
				Image patch = getRandomPatch(img, patchSize);
				td.push_back(patch);
			}
			nSamplesTaken += nSamplesThisFrame;
		}
		catch (...){}
	}
	logger("Finished reading video file, starting PCA...");
	const int inputDimension = patchSize*patchSize * 3;  // = 21*21*3 1323 by default

	proj = new zt::Projector(outputDim, td, true);
	logger("Finished PCA, saving the projector...");

	// now save it..
	proj->saveToFile(marshal_as<std::string>(fileName));
	logger("Wrote the projector to " + fileName);

}

Projector::~Projector(){
	delete proj;
}


array<ImageSource^>^ Projector::getPrincipals(){
	if (proj){
		int n = proj->outputDim();
		auto result = gcnew array<ImageSource^>(n+1);
		auto unit = gcnew array<float>(n);
		for (int i = 0; i < unit->Length; i++) unit[i] = 0.0;
		result[0] = reconstruct(unit);
		for (int i = 0; i < n; i++){
			unit[i] = sqrt(proj->get_eigenvalue(i))*3.0;
			result[i+1] = reconstruct(unit);
			unit[i] = 0.0;
		}
		return result;
	}
	return nullptr;
}

ImageSource^ Projector::reconstruct(array<float>^ descriptor){
	pin_ptr<float> data = &descriptor[0];
	float* pdata = data;
	std::vector<float> d = { pdata, pdata + (descriptor->Length) };
	Image p = proj->reconstruct(d);
	const uchar*pd = p->data();
	return BitmapSource::Create(p->width(), p->height(), 96, 96, PixelFormats::Bgr24, nullptr,
		System::IntPtr((void*)p->data()), p->num_pixels()*p->pixel_size(), p->stride());
}

array<float>^ Projector::getFeatures(int frame_number, int patch_x, int patch_y, int patch_size, FrameSource^ video){
	Image img = video->GetFrameSource().getFrame(frame_number);
	Image patch = img->subImage(patch_x, patch_y, patch_size, patch_size);
	auto f = proj->project(patch);
	auto len = f.size();
	auto result = gcnew array<float>(len);
	for (size_t i = 0; i < len; i++) result[i] = f[i];
	return result;
}
