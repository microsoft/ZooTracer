#include "Stdafx.h"
#include "KDTreeSource.h"

using namespace ztWpf;
using zt::FrameIndex;
using System::Runtime::InteropServices::Marshal;

Matches^ KDTreeSource::getMatches(FrameIndex frameNumber, array<float>^ features, int count, Projector^ projector){
	auto result = gcnew Matches();
	if (impl.is_ready(frameNumber)){
		pin_ptr<float> ptr = &features[0];
		std::vector<float> desc((float*)ptr, (float*)ptr + features->Length);
		auto matches = impl[frameNumber].get()->getMatches(desc, count);
		for (auto match : matches)
			result->Add(System::Tuple::Create(std::get<0>(match), std::get<1>(match), std::get<2>(match)));
	}
	return result;
}

bool KDTreeSource::IsReady(FrameIndex frameNumber){
	return impl.is_ready(frameNumber);
}

void KDTreeSource::Subscribe(ProgressHandler^ handler){
	if (handler == nullptr)
		impl.subscribe(nullptr);
	else{
		auto ptr = Marshal::GetFunctionPointerForDelegate(handler);
		impl.subscribe(static_cast<zt::KDTreeSource::ProgressHandler>(ptr.ToPointer()));
	}
	this->handler = handler; // keep the pointer to prevent garbage collection of the delegate
}