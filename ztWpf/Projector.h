#pragma once

#include "FrameSource.h"

#include <ztProjector.h>
#include <msclr\marshal_cppstd.h>

namespace ztWpf{
	using System::Windows::Media::ImageSource;

	public ref class Projector
	{
	private:
		zt::Projector* proj = nullptr;
	public:
		Projector(
			FrameSource^ frameSource,
			int output_dimension,
			int patch_size,
			int number_of_samples,
			System::String^ fileName,
			System::Action<System::String^>^ logger);
		Projector(System::String^ fileName);
		~Projector();
		array<ImageSource^>^ getPrincipals();
		ImageSource^ reconstruct(array<float>^);
		const zt::Projector& GetProjector(){ return *proj; }
		array<float>^ getFeatures(int frame_number, int patch_x, int patch_y, int patch_size, FrameSource^ video);
	};
}