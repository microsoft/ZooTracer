#include "TracePoint.h"

#include <ztLog.h>
#include <sstream>

#include <cassert>
#include <algorithm>
#include <typeinfo>

using namespace zt;
using std::pow;

double dist2(const Patch& that, const Patch& prev){
	return pow(double(that.x() - prev.x()), 2) + pow(double(that.y() - prev.y()), 2);
}

double dist2(const Descriptor& that, const Descriptor& other){
	assert(that.size() == other.size());
	double sum = 0;
	for (size_t i = 0; i < that.size(); i++)
		sum += pow(double(that[i] - other[i]), 2);
	return sum;
}


// error of becoming visible going from 'prev' to 'that' through 'delta' occlusion points
double TracePointAuto::become_visible(const Match& that, const Match& prev, int delta, const OptimizationParameters& pars)
{
	return that.isNull() ? (prev._pl.best_error + pars.lambda_r() - pars.lambda_o()) // we cancel occlusion penalties on open ends
		: (prev._pl.best_error
		+ pars.lambda_d()*dist2(that._location, prev._location) / (delta + 1)
		+ that._appearance);
}

double TracePointAuto::remain_visible(const Match& that, const Match& prev, const OptimizationParameters& pars)
{
	return that.isNull() ? (prev._pl.best_error)
		: (prev._pl.best_error
		+ pars.lambda_d()*dist2(that._location, prev._location)
		+ pars.lambda_u()*dist2(that._descriptor, prev._descriptor)
		+ that._appearance);
}

// For the match 'end' find 'best error' and corresponding back-tracking indices
TracePointAuto::Payload TracePointAuto::dp_update(const SegmentType& segment, size_t length, const TracePoint& start, const Match& end, const OptimizationParameters& pars){

	// error of going through "that_m" at the segment point "i"
	double e_min = std::numeric_limits<double>::max();
	// first, consider staying visible
	int i_segm = length - 1;
	int i_match = -1; // a match in 'segment[i_segm]' corresponding to 'e_min'
	auto& prev = *segment[length - 1];
	for (size_t i_m = 0; i_m < prev._matches.size(); ++i_m) // compare with "remain visible" options
	{
		double e = remain_visible(end, prev._matches[i_m], pars);
		if (e < e_min){
			e_min = e;
			i_match = i_m;
		}
	}
	// second, consider going back through occlusions
	int j = length - 1;
	int last_segm = std::max(0, static_cast<int>(length)-pars.max_occlusion_duration());
	while (last_segm>0 && segment[last_segm]->_matches.size() == 0) last_segm -= 1;
	double e_occl = pars.lambda_o() - pars.lambda_r();
	while (--j >= last_segm){
		e_occl += pars.lambda_r();
		if (e_occl >= e_min) break; // we won't find anything better that e_occl
		// redefined prev points to several points backwards
		auto& prev = *segment[j];
		for (size_t i_m = 0; i_m < prev._matches.size(); ++i_m) // compare with "remain visible" options
		{
			double e = e_occl + become_visible(end, prev._matches[i_m], length - j - 1, pars);
			if (e < e_min){
				e_min = e;
				i_match = i_m;
				i_segm = j;
			}
		}
	}
	if (last_segm == 0){
		// consider also starting point
		double e = (start.location() == nullptr) ? e_occl + 2 * pars.lambda_r() - pars.lambda_o() + end._appearance
			: e_occl + pars.lambda_r() + become_visible(end, Match(dynamic_cast<const TracePointKeyFrame&>(start)), length, pars);
		if (e < e_min || std::numeric_limits<double>::max() == e_min){
			e_min = e;
			i_segm = -1; // a hint to occlude everything up to the starting point
		}
	}
	return Payload(e_min, i_match, i_segm);

}

// Optimize a trace segment between two keyframes.
// Typically, start and stop are both TracePointKeyFrame.
// Use a segment element without any match to represent a force occluded frame.
void TracePointAuto::optimize(const SegmentType& segment, const TracePoint& start, const TracePoint& end, const OptimizationParameters& pars){
	if (segment.size() <= 0) throw std::invalid_argument("segment");
	if (typeid(start) != typeid(TracePointKeyFrame) && typeid(start) != typeid(TracePointOccluded)) throw std::invalid_argument("start");
	if (typeid(end) != typeid(TracePointKeyFrame) && typeid(end) != typeid(TracePointOccluded)) throw std::invalid_argument("end");

	const Patch* start_loc = start.location();
	// initialize first segment point
	{
		auto& tp = *segment[0];
		for (auto& m : tp._matches) {
			m._pl = Payload((start_loc == nullptr) ? m._appearance
				: remain_visible(m, Match(dynamic_cast<const TracePointKeyFrame&>(start)), pars), 0, -1);
		}
	}

	// forward pass
	for (size_t i = 1; i < segment.size(); ++i)
	{
		auto& that = *segment[i];
		auto& prev = *segment[i - 1];
		for (auto& that_m : that._matches)
		{
			that_m._pl = dp_update(segment, i, start, that_m, pars);
		}
	}

	// backward_pass
	const Patch* end_loc = end.location();
	Payload p_opt = dp_update(segment, segment.size(), start,
		end_loc == nullptr ? Match(dynamic_cast<const TracePointOccluded&>(end)) : Match(dynamic_cast<const TracePointKeyFrame&>(end)),
		pars);
	int j = static_cast<int>(segment.size());
	while (--j >= 0){
		while (static_cast<int>(j) > p_opt.backtrack_segm_idx){
			segment[j]->_best_match = -1; // occlude
			j -= 1;
		}
		if (j >= 0){
			auto& tp = *segment[j];
			tp._best_match = p_opt.backtrack_match_idx;
			p_opt = tp._matches[tp._best_match]._pl;
		}
	}
}

std::pair<float, FrameIndex> full_appearance_penalty(
	Descriptor& descriptor,
	std::vector<std::pair<FrameIndex, std::shared_ptr<TracePointKeyFrame>>>& keyframes)
{
	float a = static_cast<float>(dist2(descriptor, keyframes[0].second->descriptor()));
	FrameIndex cf = keyframes[0].first;
	for (size_t i = 1; i < keyframes.size(); ++i){
		auto& ik = keyframes[i];
		float a1 = std::min(a, static_cast<float>(dist2(descriptor, ik.second->descriptor())));
		if (a1 < a){
			a = a1;
			cf = ik.first;
		}
	}
	return std::pair<float, FrameIndex>{ a, cf };
}

// Combines existing matches for the frame with new matches from a new keypoint.
void TracePointAuto::add_matches(
	std::vector<std::pair<FrameIndex, std::shared_ptr<TracePointKeyFrame>>>& keyframes,
	int keyframe_to_add,
	std::vector<std::pair<Patch, Descriptor>>& matches_to_add,
	int max_matches_per_frame,
	double appearance_threshold){
	if (keyframes.size() <= 0) throw std::invalid_argument("keyframes");
	if (keyframe_to_add < 0 || keyframe_to_add >= static_cast<int>(keyframes.size())) throw std::invalid_argument("keyframe_to_add");
	FrameIndex keyframe_to_add_frame = keyframes[keyframe_to_add].first;
	//
	//  Assumes existing matches have been previously compared with all the keyframes except keyframe_to_add.
	//  Resulting matches have _appearance and _closest_keyframe properly set.
	//  Leaves no more than 'total matches count' matches with lowest _appearance.
	//
	// 1. check appearance penalty of existing matches
	for (auto& m : _matches){
		if (m._closest_keyframe == keyframe_to_add_frame) {
			auto af = full_appearance_penalty(m._descriptor, keyframes);
			m._appearance = af.first;
			m._closest_keyframe = af.second;
		} else {
			float a = static_cast<float>(dist2(m._descriptor, keyframes[keyframe_to_add].second->descriptor()));
			if (a < m._appearance){
				m._appearance = a;
				m._closest_keyframe = keyframe_to_add_frame;
			}
		}
	}
	// 2. add new matches
	for (auto& ld : matches_to_add){
		// 2.1 compute appearance penalty
		auto af = full_appearance_penalty(ld.second, keyframes);
		// 2.2 add matches to the list of possible path nodes
		if (_matches.size() == 0) {
			_matches.push_back(Match(ld.first, ld.second, af.second, af.first));
		}
		else if (af.first < appearance_threshold)
		{
			assert(_matches.size()>0);
			// detect duplicate
			auto ptr = std::find_if(_matches.begin(), _matches.end(), [&ld](Match& m) {return dist2(m._location, ld.first) < 1.0; });
			if (ptr >= _matches.end())
			{
				if (_matches.size() < max_matches_per_frame){
					_matches.push_back(Match(ld.first, ld.second, af.second, af.first));
				}
				else {
					ptr = std::max_element(_matches.begin(), _matches.end(), [](Match& m1, Match& m2){return m1._appearance < m2._appearance; });
					assert(ptr >= _matches.begin() && ptr < _matches.end());
					if (af.first < ptr->_appearance){
						ptr->_location = ld.first;
						ptr->_descriptor = ld.second;
						ptr->_closest_keyframe = af.second;
						ptr->_appearance = af.first;
					}
				}
			}
		}
	}
}