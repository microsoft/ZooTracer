#pragma once

#include <ztPatch.h>
#include <memory>
#include <vector>


namespace zt{

	using Descriptor = std::vector<float>;
	class TracePointAuto;
	using SegmentType = std::vector<std::shared_ptr<TracePointAuto>>;
	class TracePointKeyFrame;
	class TracePointOccluded;
	class TracePointAuto;


	// A trace point orresponding to a particular video frame. TracePoint = Fixed | Occluded | Optimized
	class TracePoint{
	public:
		virtual ~TracePoint(){}
		// Returns nullptr when occluded.
		virtual const Patch* location() const = 0;
		static bool is_keyframe(const TracePoint& tp) { return typeid(tp) == typeid(TracePointKeyFrame); }
		static bool is_occluded(const TracePoint& tp) { return typeid(tp) == typeid(TracePointOccluded); }
		static bool is_auto(const TracePoint& tp) { return typeid(tp) == typeid(TracePointAuto); }
	};

	// A user specified trace point.
	class TracePointKeyFrame : public TracePoint{
	private:
		Patch _location;
		Descriptor _descriptor;
	public:
		~TracePointKeyFrame() override{}
		const Patch* location() const override { return &_location; }
		const Descriptor& descriptor() const { return _descriptor; }
		TracePointKeyFrame(const Patch& location, const Descriptor& descriptor) : _location(location), _descriptor(descriptor){}
	};

	// A user specified occlusion.
	class TracePointOccluded : public TracePoint{
	public:
		~TracePointOccluded() override{}
		const Patch* location() const override { return nullptr; }
	};

	class OptimizationParameters;

	// An auto-generated trace point.
	class TracePointAuto : public TracePoint{
	private:
		class Payload{
		public:
			double best_error;
			int backtrack_match_idx;
			int backtrack_segm_idx;
			Payload(){}
			Payload(double best_error, int backtrack_match_idx, int backtrack_segm_idx) : best_error(best_error), backtrack_match_idx(backtrack_match_idx), backtrack_segm_idx(backtrack_segm_idx){}
		};
		class Match{
		public:
			Patch _location;
			Descriptor _descriptor;
			FrameIndex _closest_keyframe; // The keyframe frame index.
			float _appearance; // Penalty of deviation of appearance from the best keyframe. Usually measured as squarred distance in descriptor space
			Payload _pl;
			Match(const Patch& location, const Descriptor& descriptor, FrameIndex fi, float appearance) :
				_location(location), _descriptor(descriptor), _closest_keyframe(fi), _appearance(appearance){}
			Match(const TracePointKeyFrame& tp) : _location(*tp.location()), _descriptor(tp.descriptor()), _appearance(0.0), _pl(0.0, 0, 0){}
			Match(const TracePointOccluded& tp) : _location(0, 0), _appearance(std::numeric_limits<float>::infinity()){}
			bool isNull() const { return _appearance == std::numeric_limits<float>::infinity(); }
		};
		std::vector<Match> _matches; // Candidates to trace point location.
		int _best_match = -1; // An index of the match in an optimal trace. The value -1 indicates occlusion.

		// "become visible" error
		static double become_visible(const Match& that, const Match& prev, int delta, const OptimizationParameters&);
		static double remain_visible(const Match& that, const Match& prev, const OptimizationParameters&);
		static Payload TracePointAuto::dp_update(const SegmentType& segment, size_t length, const TracePoint& start, const Match& end, const OptimizationParameters& pars);
	public:
		~TracePointAuto() override{}
		const Patch* location() const override { return _best_match < 0 ? nullptr : &(_matches[_best_match]._location); }
		void add_match(const Patch& location, const Descriptor& descriptor, FrameIndex fi, float appearance){ _matches.push_back(Match(location, descriptor, fi, appearance)); }
		void add_matches(
			std::vector<std::pair<FrameIndex, std::shared_ptr<TracePointKeyFrame>>>& keyframes,
			int keyframe_to_add,
			std::vector<std::pair<Patch, Descriptor>>& matches_to_add,
			int max_matches_per_frame,
			double appearance_threshold);

		// The algorithm updates trace points contents in the segment.
		static void optimize(const SegmentType& segment, const TracePoint& start, const TracePoint& end, const OptimizationParameters&);
	};

	class OptimizationParameters{
	private:
		double _lambda_d;
		double _lambda_u;
		double _lambda_o;
		int _max_occlusion_duration = 250;
	public:
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
		OptimizationParameters(double λ_d, double λ_u, double λ_o, int max_occlusion_duration) 
			:_lambda_d(λ_d), _lambda_u(λ_u), _lambda_o(λ_o), _max_occlusion_duration(max_occlusion_duration) {}
	};
}