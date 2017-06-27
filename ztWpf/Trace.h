#pragma once

#include <ztTrace.h>

#include "Patch.h"
#include "KDTreeSource.h"


namespace ztWpf{

	using zt::FrameIndex;

	// Holds a vector of trace points for a video
	public ref class Trace
	{
	public:
		Trace(KDTreeSource^ kdtree_source,
			double match_ratio, int num_matches, int max_matches_per_frame, double matches_appearance_threshold,
			double λ_d, double λ_u, double λ_o, int max_occlusion_duration);
		~Trace();

		/// <summary>Get location of the traced object at the frame. Returns nullptr if the object is occluded.</summary>
		Patch^ GetTracePoint(FrameIndex frameNumber);

		/// <summary>Set traced object location at the frame.</summary>
		void Fix(FrameIndex frameNumber, Patch^ location, array<float>^ descriptor);
		bool IsFixed(FrameIndex frameNumber);

		/// <summary>Set traced object occluded at the frame.</summary>
		void Occlude(FrameIndex frameNumber);
		bool IsForceOccluded(FrameIndex frameNumber);

		/// <summary>Clear trace point/occlusion information for the frame.</summary>
		void Clear(FrameIndex frameNumber);

		///<summary>Rerun the whole trace optimization with current trace points.</summary>
		void Rerun(double match_ratio, int num_matches, int max_matches_per_frame, double matches_appearance_threshold,
			double λ_d, double λ_u, double λ_o, int max_occlusion_duration);

		delegate void ProgressHandler(int, int);

		///<summary>Subscribe to notifications about trace build progress.</summary>
		void Subscribe(ProgressHandler^);
		
	private:
		zt::Trace& impl;
		ProgressHandler^ handler;
	};
}
