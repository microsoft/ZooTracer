#include <ztKDTree.h>
#include <ztLog.h>

#include <vector>
using std::tuple;
using std::get;
using std::vector;
using std::shared_ptr;
using std::make_shared;

#include <sstream>
using std::ostringstream;

#include <future>
using std::shared_future;
using std::promise;
#include <chrono>
using std::chrono::duration;

#include <agents.h>
using concurrency::agent;

#include <bounded_queue.h>

// an implementation that generates trees in background thread and keeps them in memory.
namespace zt{
	class SimpleKDTreeSource::implementation : public agent
	{
	public:
		using FrameMsg = tuple<Image, promise<shared_ptr<KDTree>>*, FrameIndex>;
		implementation(OpenCVFrameSource& video, const Projector& projector, int pixel_step, int number_of_workers);
		~implementation();
		shared_ptr<KDTree> operator [] (FrameIndex idx) const {return _futures[idx].get();}
		bool is_ready(FrameIndex idx) const { return _futures[idx].wait_for(duration<int>::zero()) == std::future_status::ready; }
		void subscribe(KDTreeSource::ProgressHandler handler){ if (!_stopping){ _subscription = handler; handler(_complete_count); } }
		int num_frames() const { return static_cast<int>(_futures.size()); }
	private:
		vector<promise<shared_ptr<KDTree>>> _promises;
		vector<shared_future<shared_ptr<KDTree>>> _futures;
		bool _stopping;
		void run() override;
		OpenCVFrameSource& _video;

		class KDtreeFactoryAgent : public agent{
		public:
			KDtreeFactoryAgent(bounded_queue<FrameMsg>& source, const Projector& projector, int pixel_step) :_source(source), _projector(projector), _pixel_step(pixel_step){ start(); }
			void run() override;
		private:
			bounded_queue<FrameMsg>& _source;
			const Projector& _projector;
			int _pixel_step;
		};

		bounded_queue<FrameMsg> _frame_queue;
		vector<shared_ptr<KDtreeFactoryAgent>> _workers;
		KDTreeSource::ProgressHandler _subscription;
		FrameIndex _complete_count;
	};
}
using namespace zt;

SimpleKDTreeSource::SimpleKDTreeSource(OpenCVFrameSource& video, const Projector& projector, int pixel_step, int number_of_workers)
: impl(new implementation(video, projector, pixel_step, number_of_workers)){}
SimpleKDTreeSource::~SimpleKDTreeSource() {}
shared_ptr<KDTree> SimpleKDTreeSource::operator [] (FrameIndex idx) const { return (*impl)[idx]; }

bool SimpleKDTreeSource::is_ready(FrameIndex idx) const { return impl->is_ready(idx); }
int SimpleKDTreeSource::num_frames() const { return impl->num_frames(); }
void SimpleKDTreeSource::subscribe(ProgressHandler handler) { impl->subscribe(handler); }


SimpleKDTreeSource::implementation::implementation(OpenCVFrameSource& video, const Projector& projector, int pixel_step, int number_of_workers)
: _video(video), _frame_queue(number_of_workers), _stopping(false), _subscription(nullptr), _complete_count(0) {
	int len = video.numFrames();
	for (int i = 0; i < len; i++){
		_promises.push_back(promise<shared_ptr<KDTree>>{});
		_futures.push_back(_promises[_promises.size() - 1].get_future().share());
	}
	for (int w = 0; w < number_of_workers; w++){
		_workers.push_back(make_shared<KDtreeFactoryAgent>( _frame_queue, projector, pixel_step ));
	}
	start();
}

SimpleKDTreeSource::implementation::~implementation(){
	_stopping = true;
	_subscription = nullptr;
	wait(this);
	Log::write("kd-tree source destroyed");
}

void SimpleKDTreeSource::implementation::run() {
	KDTreeSource::ProgressHandler notify;
	for (FrameIndex i = 0; i < _video.numFrames() && !_stopping; i++){
		//ostringstream s; s << "starting kd-tree " << i << "...";
		//_logger(s.str());
		try{
			_frame_queue.enqueue(FrameMsg{ _video.getFrame(i), &_promises[i], i });
			FrameIndex prev_count = _complete_count;
			while (_complete_count < _futures.size() && is_ready(_complete_count)) _complete_count += 1;
			notify = _subscription;
			if (notify != nullptr && _complete_count>prev_count) notify(_complete_count);
		}
		catch (std::exception e){
			ostringstream s; s << "exception while getting a frame " << i << ": " << e.what();
			Log::write(s.str());
		}
	}
	// stop all workers
	if (_stopping) {
		FrameMsg buf;
		while(_frame_queue.try_dequeue(buf));
	}
	for (size_t i = 0; i < _workers.size(); i++)
		_frame_queue.enqueue(FrameMsg{ Image{}, nullptr, -1 });
	for (auto& w : _workers) wait(&(*w));
	_complete_count = static_cast<FrameIndex>(_futures.size());
	notify = _subscription;
	if (notify != nullptr) notify(_complete_count);
	done();
}

void SimpleKDTreeSource::implementation::KDtreeFactoryAgent::run() {
	while (true){
		FrameMsg m = _source.dequeue();
		if (get<2>(m) < 0) break;
		Image img = get<0>(m);
		try{
			get<1>(m)->set_value(make_shared<KDTree>(img, _projector, _pixel_step));
			ostringstream s; s << "done kd-tree " << get<2>(m) << ".";
			Log::write(s.str());
		}
		catch (std::exception e)
		{
			ostringstream s; s << "exception while building kd-tree " << get<2>(m) << ": " << e.what();
			Log::write(s.str());
		}
	}
	done();
}