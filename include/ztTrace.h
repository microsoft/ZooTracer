#pragma once
#include <ztKDTree.h>

#include <memory>

namespace zt{
	class TraceParameters;

	class Trace{
	public:
		~Trace();
		using Handler = void(__stdcall *)(FrameIndex start, FrameIndex end);

		Trace(const KDTreeSource& source, const TraceParameters&);

		// Fetches location of the traced object at the frame. Returns nullptr if the traced object is occluded at the frame.
		const Patch* get_tracepoint(FrameIndex) const;

		// Add or change fixed tracepoint and asynchronously start building a new trace.
		void fix(FrameIndex, Patch, const Descriptor&);

		bool is_fixed(FrameIndex);

		// Force occlusion and asynchronously start building a new trace.
		void occlude(FrameIndex);

		bool is_forced_occluded(FrameIndex);

		// Clear user directives for the frame.
		void reset(FrameIndex);

		// Rerun the whole trace optimization.
		void rerun(const TraceParameters&);

		// The subscriber will get notifications when new trace segments are available. 
		// The 'nullptr' value clears the subscription. The handler may be called from a different thread.
		void subscribe(Handler);
	private:
		class Implementation; std::unique_ptr<Implementation> impl;
	};

	class TraceParameters{
	public:
		// Number of approximate matches to take for each key frame.
		int num_matches() const { return _num_matches; }

		// Approximate ratio for searching matches.
		double match_ratio() const { return _match_ratio; }

		int max_matches_per_frame() const { return _max_matches_per_frame; }

		double appearance_threshold() const { return _appearance_threshold; }

		// Velocity penalty.
		double lambda_d() const{ return _lambda_d; }

		// Appearance update penalty.
		double lambda_u() const{ return _lambda_u; }

		// Become occluded penalty.
		double lambda_o() const{ return _lambda_o; }

		// Remain occluded penalty.
		double lambda_r() const
		{
			return 0.5*lambda_o();
		}

		// Number of frames
		int max_occlusion_duration() const{ return _max_occlusion_duration; }
		TraceParameters(int num_matches, double match_ratio, int max_matches_per_frame, double appearance_threshold, double λ_d, double λ_u, double λ_o, int max_occlusion_duration)
			:_num_matches(num_matches), _match_ratio(match_ratio), _max_matches_per_frame(max_matches_per_frame), _appearance_threshold(appearance_threshold), _lambda_d(λ_d), _lambda_u(λ_u), _lambda_o(λ_o), _max_occlusion_duration(max_occlusion_duration){}
	private:
		int _num_matches;
		double _match_ratio;
		int _max_matches_per_frame;
		double _appearance_threshold;
		double _lambda_d;
		double _lambda_u;
		double _lambda_o;
		int _max_occlusion_duration;
	};

}