#pragma once

#include "Projector.h"

#include <ztKDTree.h>
#include <msclr/marshal.h>

namespace ztWpf{

	using zt::FrameIndex;
	using Matches = System::Collections::Generic::List<System::Tuple<int, int, double>^>;
	using System::String;
	using msclr::interop::marshal_as;

	public ref class KDTreeSource{
	public:
		KDTreeSource(FrameSource^ video, Projector^ projector, int pixel_step, int number_of_workers, String^ folder_path)
			: impl(*(new zt::FileKDTreeSource(video->GetFrameSource(), projector->GetProjector(), pixel_step, number_of_workers, marshal_as<std::string>(folder_path)))){}
		~KDTreeSource(){ delete &impl; }
		const zt::KDTreeSource& GetKDTreeSource(){ return impl; }
		Matches^ getMatches(FrameIndex frameNumber, array<float>^ features, int count, Projector^ projector);
		bool IsReady(FrameIndex frameNumber);
		delegate void ProgressHandler(FrameIndex);
		void Subscribe(ProgressHandler^ handler);
	private:
		zt::KDTreeSource& impl;
		ProgressHandler^ handler;
	};
}
