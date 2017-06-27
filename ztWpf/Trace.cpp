#include "stdafx.h"
#include "Trace.h"



using namespace ztWpf;
using zt::FrameIndex;
using System::Runtime::InteropServices::Marshal;

Trace::Trace(KDTreeSource^ kdtree_source, 
	double match_ratio, int num_matches, int max_matches_per_frame, double appearance_threshold,
	double λ_d, double λ_u, double λ_o, int max_occlusion_duration)
	: impl(*(new zt::Trace(kdtree_source->GetKDTreeSource(), zt::TraceParameters(num_matches, match_ratio, max_matches_per_frame, appearance_threshold, λ_d, λ_u, λ_o, max_occlusion_duration)))) {}

Trace::~Trace() {
	impl.subscribe(nullptr);
	delete &impl; 
}

Patch^ Trace::GetTracePoint(FrameIndex frameNumber){
	auto p = impl.get_tracepoint(frameNumber);
	return p == nullptr ? nullptr : gcnew Patch(*p);
}
void Trace::Fix(FrameIndex frameNumber, Patch^ location, array<float>^ descriptor){
	zt::Patch p = { location->x, location->y };
	pin_ptr<float> data = &descriptor[0];
	float* pdata = data;
	zt::Descriptor d = { pdata, pdata + (descriptor->Length) };
	impl.fix(frameNumber, p, d);
}
bool Trace::IsFixed(FrameIndex frameNumber){ return impl.is_fixed(frameNumber); }
void Trace::Occlude(FrameIndex frameNumber){ impl.occlude(frameNumber); }
bool Trace::IsForceOccluded(FrameIndex frameNumber){ return impl.is_forced_occluded(frameNumber); }
void Trace::Clear(FrameIndex frameNumber){ impl.reset(frameNumber); }
void Trace::Rerun(double match_ratio, int num_matches, int max_matches_per_frame, double appearance_threshold,
	double λ_d, double λ_u, double λ_o, int max_occlusion_duration){
	impl.rerun(zt::TraceParameters(num_matches, match_ratio, max_matches_per_frame, appearance_threshold, λ_d, λ_u, λ_o, max_occlusion_duration));
}

void Trace::Subscribe(ProgressHandler^ handler){
	if (handler == nullptr)
		impl.subscribe(nullptr);
	else{
		auto ptr = Marshal::GetFunctionPointerForDelegate(handler);
		impl.subscribe(static_cast<zt::Trace::Handler>(ptr.ToPointer()));
	}
	this->handler = handler; // keep the pointer to prevent garbage collection of the delegate
}