#include <cassert>

#include <ztTrace.h>
#include <ztLog.h>
#include <sstream>

#include "TracePoint.h"

#include <ppl.h>
#include <agents.h>
using concurrency::task_group;

using namespace zt;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::tuple;
using std::get;
using std::pair;

// Builds and holds one trace.
class zt::Trace::Implementation : concurrency::agent{
public:
	Implementation(const KDTreeSource& kdtree_source, const TraceParameters&);
	~Implementation();

	const Patch* get_tracepoint(FrameIndex) const;
	void fix(FrameIndex, Patch, const Descriptor&);
	bool is_fixed(FrameIndex);
	void occlude(FrameIndex);
	bool is_forced_occluded(FrameIndex);
	void reset(FrameIndex);
	void rerun(const TraceParameters&);
	void subscribe(Handler event_handler){ _subscription = event_handler; }
protected:
	void run() override;
private:
	TraceParameters _pars;
	const KDTreeSource& _kdtree_source;
	vector<shared_ptr<TracePoint>> _trace;
	Handler _subscription; // notifies a change in a trace segment.
	task_group _build; // builds trace segments in parallel.
	void start_build(); // starts trace optimization.
	void cancel_build(); // cancel building the trace.
	using Msg = tuple<FrameIndex, shared_ptr<TracePoint>>;
	static Msg make_msg(FrameIndex i, TracePoint* tp){ return std::make_tuple(i, shared_ptr<TracePoint>(tp)); }
	concurrency::unbounded_buffer<Msg> _jobs;  // accepts jobs from 'fix', 'occlude' and 'reset' .
	void change_the_trace(FrameIndex, shared_ptr<TracePoint>); // make changes to the trace according to user input.
	shared_ptr<TracePointAuto> _occlusion{ new TracePointAuto() };
	shared_ptr<TracePointOccluded> _trace_end{ new TracePointOccluded() };
	bool is_keyframe(FrameIndex frame){ return TracePoint::is_keyframe(*_trace[frame]); }
	bool is_occluded(FrameIndex frame){ return TracePoint::is_occluded(*_trace[frame]); }
	bool is_auto(FrameIndex frame){ return TracePoint::is_auto(*_trace[frame]); }
};

Trace::Trace(const KDTreeSource& source, const TraceParameters& pars) :impl(new Implementation(source, pars)){}
Trace::~Trace(){}

// Get location of the traced object at the frame. Returns nullptr if the traced object is occluded at the frame.
const Patch* Trace::get_tracepoint(FrameIndex frame) const{ return impl->get_tracepoint(frame); }
void Trace::fix(FrameIndex frame, Patch location, const Descriptor& descriptor){ impl->fix(frame, location, descriptor); }
bool Trace::is_fixed(FrameIndex frame){ return impl->is_fixed(frame); }
void Trace::occlude(FrameIndex frame){ impl->occlude(frame); }
bool Trace::is_forced_occluded(FrameIndex frame){ return impl->is_forced_occluded(frame); }
void Trace::reset(FrameIndex frame){ impl->reset(frame); }
void Trace::rerun(const TraceParameters& pars){ impl->rerun(pars); }

// Links notification handler to the trace. The 'nullptr' value clears the subscription. The handler may be called from a different thread.
void Trace::subscribe(Handler event_handler){ impl->subscribe(event_handler); }


Trace::Implementation::Implementation(const KDTreeSource& kdtree_source, const TraceParameters& pars)
: _kdtree_source(kdtree_source), _subscription(nullptr), _pars(pars)
{
	int n = _kdtree_source.num_frames();
	for (int i = 0; i < n; i++) _trace.push_back(std::make_shared<TracePointAuto>());
	start(); // starts the agent (see run() method).
}
Trace::Implementation::~Implementation() {
	_jobs.enqueue(make_msg(-1, new TracePointOccluded()));
	wait(this);
	Log::write("trace destroyed");
}


// returns trace point location or nullptr if occluded.
const Patch* Trace::Implementation::get_tracepoint(FrameIndex frame) const{
	if (frame < 0 || frame >= static_cast<FrameIndex>(_trace.size())) throw std::out_of_range("frame");
	return _trace[frame]->location();
}

void Trace::Implementation::fix(FrameIndex frame, Patch location, const Descriptor& descriptor){
	_jobs.enqueue(make_msg(frame, new TracePointKeyFrame{ location, descriptor }));
}

bool Trace::Implementation::is_fixed(FrameIndex frame){ return is_keyframe(frame); }

void Trace::Implementation::occlude(FrameIndex frame){
	if (!is_occluded(frame))
		_jobs.enqueue(make_msg(frame, new TracePointOccluded()));
}

bool Trace::Implementation::is_forced_occluded(FrameIndex frame){ return is_occluded(frame); }

void Trace::Implementation::reset(FrameIndex frame){
	if (!is_auto(frame))
		_jobs.enqueue(make_msg(frame, new TracePointAuto()));
}

void Trace::Implementation::rerun(const TraceParameters& pars){
	_pars = pars;
	_jobs.enqueue(make_msg(0, nullptr)); }


// agent message loop
void Trace::Implementation::run(){
	try{
		while (true)
		{
			auto msg = concurrency::receive(_jobs);
			cancel_build();
			if (get<0>(msg) < 0) break;
			change_the_trace(get<0>(msg), move(get<1>(msg)));
			while (concurrency::try_receive(_jobs, msg))
			{
				if (get<0>(msg) < 0) break;
				change_the_trace(get<0>(msg), move(get<1>(msg)));
			}
			start_build();
		}
	}
	catch (std::exception e)
	{
		Log::write("Exception in trace agent: " + std::string(e.what()));
	}
	done();
}

void Trace::Implementation::change_the_trace(FrameIndex frame, shared_ptr<TracePoint> tp){
	bool add = false;
	bool all = false;
	if (nullptr == tp){
		add = all = true;
	}
	else if (TracePoint::is_occluded(*tp)){
		add = all = is_keyframe(frame);
		_trace[frame] = std::make_shared<TracePointOccluded>();
	}
	else if (TracePoint::is_auto(*tp) && !is_auto(frame)){
		add = all = true;
		_trace[frame] = std::make_shared<TracePointAuto>();
	}
	else if (TracePoint::is_keyframe(*tp)){
		add = true;
		all = is_keyframe(frame);
		_trace[frame] = make_shared<TracePointKeyFrame>(std::move(dynamic_cast<TracePointKeyFrame&>(*tp.get())));
	}
	else throw std::exception("internal error");
	if (add && all){
		// full trace preparation
		for (size_t i = 0; i < _trace.size(); i++){
			if (is_auto(i)){
				auto p = make_shared<TracePointAuto>(); // reset the point
				if (_kdtree_source.is_ready(i)){
					auto kdt = _kdtree_source[i];
					vector<pair<FrameIndex, shared_ptr<TracePointKeyFrame>>> kfs;
					for (size_t k = 0; k < _trace.size(); k++){
						if (is_keyframe(k)){
							int key_frame = static_cast<int>(kfs.size());
							kfs.push_back(pair<FrameIndex, shared_ptr<TracePointKeyFrame>>(k, dynamic_pointer_cast<TracePointKeyFrame>(_trace[k])));
							auto kd_matches = kdt->getMatches(kfs[key_frame].second->descriptor(), _pars.num_matches(), _pars.match_ratio());
							vector<pair<Patch, Descriptor>> tp_matches;
							for (auto& m : kd_matches)
								tp_matches.push_back(pair<Patch, Descriptor>(Patch(get<0>(m), get<1>(m)), get<3>(m)));
							p->add_matches(kfs, key_frame, tp_matches, _pars.max_matches_per_frame(), _pars.appearance_threshold());
						}
					}
				}
				_trace[i] = move(p);
			}
		}
	}
	else if (add){
		// incremental trace preparation
		for (size_t i = 0; i < _trace.size(); i++){
			if (is_auto(i)){
				if (_kdtree_source.is_ready(i)){
					auto kdt = _kdtree_source[i];
					vector<pair<FrameIndex, shared_ptr<TracePointKeyFrame>>> kfs;
					int key_frame = -1;
					for (size_t k = 0; k < _trace.size(); k++){
						if (is_keyframe(k)){
							kfs.push_back(pair<FrameIndex, shared_ptr<TracePointKeyFrame>>(k, dynamic_pointer_cast<TracePointKeyFrame>(_trace[k])));
							if (k == frame) key_frame = static_cast<int>(kfs.size()) - 1;
						}
					}
					assert(key_frame >= 0);
					auto kd_matches = kdt->getMatches(kfs[key_frame].second->descriptor(), _pars.num_matches(), _pars.match_ratio());
					vector<pair<Patch, Descriptor>> tp_matches;
					for (auto& m : kd_matches)
						tp_matches.push_back(pair<Patch, Descriptor>(Patch(get<0>(m), get<1>(m)), get<3>(m)));
					dynamic_pointer_cast<TracePointAuto>(_trace[i])->add_matches(kfs, key_frame, tp_matches, _pars.max_matches_per_frame(), _pars.appearance_threshold());
				}
			}
		}
	}
}

// function object
class BuildTask{
public:
	BuildTask(shared_ptr<TracePoint> before, FrameIndex segment_start, const OptimizationParameters& pars, Trace::Handler handler)
		: _before(before), _segment_start(segment_start), _pars(pars), _handler(handler){}

	void operator() () const {
		FrameIndex segment_end = _segment_start + static_cast<FrameIndex>(_segment.size()) - 1;
		{
			std::stringstream ss;
			ss << "starting segment " << _segment_start << " to " << segment_end;
			Log::write(ss.str());
		}
		try{
			if (_handler != nullptr) _handler(_segment_start, segment_end);
			TracePointAuto::optimize(_segment, *_before, *_after, _pars);
			if (_handler != nullptr) _handler(_segment_start, segment_end);
		}
		catch (std::exception e){
			Log::write("exception in trace optimization: " + std::string(e.what()));
		}
		{
			std::stringstream ss;
			ss << "finished segment " << _segment_start << " to " << segment_end;
			Log::write(ss.str());
		}
	}

	void add(shared_ptr<TracePointAuto> trace_point){ _segment.push_back(trace_point); }
	void after(shared_ptr<TracePoint> after){ _after = after; }
private:
	FrameIndex _segment_start;
	SegmentType _segment;
	shared_ptr<TracePoint> _before;
	shared_ptr<TracePoint> _after;
	OptimizationParameters _pars;
	Trace::Handler _handler;
};

void Trace::Implementation::start_build(){
	shared_ptr<TracePoint> start = _trace_end;
	FrameIndex segment_start = 0;
	FrameIndex frame_end = static_cast<FrameIndex>(_trace.size());
	while (segment_start < frame_end && _kdtree_source.is_ready(segment_start))
	{
		while (segment_start < frame_end && _kdtree_source.is_ready(segment_start) && is_keyframe(segment_start))
		{
			start = _trace[segment_start];
			segment_start += 1;
		}
		if (segment_start < frame_end && _kdtree_source.is_ready(segment_start))
		{
			BuildTask task(start, segment_start, OptimizationParameters(_pars.lambda_d(), _pars.lambda_u(), _pars.lambda_o(), _pars.max_occlusion_duration()), _subscription);
			while (segment_start < frame_end && _kdtree_source.is_ready(segment_start) && !(is_keyframe(segment_start)))
			{
				task.add(is_auto(segment_start) ? dynamic_pointer_cast<TracePointAuto>(_trace[segment_start]) : _occlusion);
				++segment_start;
			}
			task.after(segment_start < frame_end && _kdtree_source.is_ready(segment_start) ? _trace[segment_start] : _trace_end);
			_build.run(task);
		}
	}
}

void Trace::Implementation::cancel_build(){
	_build.cancel();
	_build.wait();
}