#include <OpenCVFrameSource.h>
#include "OpenCVImage.h"

#include <opencv2/highgui/highgui.hpp> 
#include <future>
#include <agents.h>


namespace zt {
	class OpenCVFrameSource::implementation : concurrency::agent
	{
		//friend class OpenCVFrameSource;
	public:
		implementation();
		implementation(std::string const & filename);
		~implementation();

		void setFrameOffset(int);
		void openVideo(std::string const & filename);
		int numFrames() const;
		bool isActive() const;

		// TODO: if n < frame_count, cache preceding N frames
		Image getFrameData(int);

		int getFrameWidth() const;
		int getFrameHeight() const;
		double getFPS() const;
		std::string getFourCC() const;
		std::future<Image> post(int);

	private:
		cv::VideoCapture source;
		std::string video_filename;
		int frame_pointer;
		int frame_count;
		int frame_offset;
		int frame_width;
		int frame_height;
		double fps;
		std::string fourCC;

		void init();

		// trivial cache
		int last_frame_number = -1;
		Image last_frame;
		Image getFrame(int);

		//class OCVVH_cache : public MemCache<OpenCVImage>
		//{
		//private:
		//	implementation * parent;
		//	void getCallback(int n, OpenCVImage & b) { return parent->getFrame(n, b); }
		//public:
		//	OCVVH_cache(implementation * owner) : parent(owner) {}
		//	~OCVVH_cache() {}
		//} *cache;

		void openVideoFile(std::string const & filename);

		// asynchronous mailbox
		typedef std::pair<int, std::promise<Image>> msg_type;
		concurrency::unbounded_buffer<std::shared_ptr<msg_type>> mqueue;
		void run() override;
	};

} // end namespace AmpTrack

using namespace zt;

OpenCVFrameSource::OpenCVFrameSource(std::string fileName) : impl(new OpenCVFrameSource::implementation(fileName)) {}
OpenCVFrameSource::~OpenCVFrameSource() {}
int OpenCVFrameSource::numFrames() const { return impl->numFrames(); }
//bool OpenCVFrameSource::isActive() const { return impl->isActive(); }
Image OpenCVFrameSource::getFrame(int n) const { return impl->post(n).get(); }
int OpenCVFrameSource::frameWidth() const { return impl->getFrameWidth(); }
int OpenCVFrameSource::frameHeight() const { return impl->getFrameHeight(); }
double OpenCVFrameSource::framesPerSecond() const { return impl->getFPS(); }
std::string OpenCVFrameSource::fourCC() const { return impl->getFourCC(); }

void OpenCVFrameSource::implementation::init()
{
	//frame_pointer = Nullint;
	//frame_offset = 0;
	//cache = new OCVVH_cache(this);
}

OpenCVFrameSource::implementation::implementation()
:mqueue()
{
	init();
}
OpenCVFrameSource::implementation::implementation(std::string const & filename)
{
	init();
	this->openVideo(filename);
}
OpenCVFrameSource::implementation::~implementation()
{
	//delete cache;
}

bool OpenCVFrameSource::implementation::isActive() const
{
	return source.isOpened();
}

void OpenCVFrameSource::implementation::setFrameOffset(int n)
{
	frame_offset = n;
}

void OpenCVFrameSource::implementation::openVideo(std::string const & filename)
{
	// TODO: check whether 'filename' already open ...would need explicit filename/frame_pointer reset below. 
	openVideoFile(filename);
}

void OpenCVFrameSource::implementation::openVideoFile(std::string const & filename)
{
	// TODO: sort out video_filename member - should reflect actuality, not aspiration
	source.open(filename);

	if (!source.isOpened())
	{
		//printf("DEBUG: failed to open '%s'\n",filename);
		video_filename = "";
		frame_pointer = 0;
		frame_count = 0;
	}
	else
	{
		//printf("DEBUG: successfully opened '%s'\n",filename.c_str());
		video_filename = filename;
		//cache->reset();

		// force codec to actually count frames. The fetch stops past the last frame.
		source.set(CV_CAP_PROP_POS_FRAMES, source.get(CV_CAP_PROP_FRAME_COUNT));
		frame_count = static_cast<int>(source.get(CV_CAP_PROP_POS_FRAMES));

		// set-up frame buffer
		frame_width = static_cast<int>(source.get(CV_CAP_PROP_FRAME_WIDTH));
		frame_height = static_cast<int>(source.get(CV_CAP_PROP_FRAME_HEIGHT));
		fps = source.get(CV_CAP_PROP_FPS);
		int fcc = static_cast<int>(source.get(CV_CAP_PROP_FOURCC));
		fourCC.clear();
		fourCC.push_back(char(fcc & 255));
		fourCC.push_back(char((fcc >> 8) & 255));
		fourCC.push_back(char((fcc >> 16) & 255));
		fourCC.push_back(char((fcc >> 24) & 255));

		// TODO: validity checks
		/*
		cv::Mat frame;
		source.grab();
		source.retrieve(frame);
		*/
	}
}

int OpenCVFrameSource::implementation::numFrames() const
{
	return frame_count;
}

int OpenCVFrameSource::implementation::getFrameWidth() const
{
	return frame_width;
}

int OpenCVFrameSource::implementation::getFrameHeight() const
{
	return frame_height;
}

double OpenCVFrameSource::implementation::getFPS() const { return fps; }
std::string OpenCVFrameSource::implementation::getFourCC() const { return fourCC; }

//Image OpenCVFrameSource::implementation::getFrameData(int n)
//{
//	n += frame_offset;
//
//	if (n != frame_pointer)
//	{
//		frame_buffer = (*cache)[n];
//		frame_pointer = n;
//	}
//
//	return *frame_buffer;
//}

Image OpenCVFrameSource::implementation::getFrame(int n)
{
	if (!source.isOpened() || n < 0 || n >= numFrames()) throw std::exception("invalid operation on OpenCV video capture");
	if (n != last_frame_number){
		last_frame_number = n;
		source.set(CV_CAP_PROP_POS_FRAMES, n);

		cv::Mat frame;
		source >> frame;
		last_frame = std::make_shared<OpenCVImage>(frame);
	}
	return last_frame;
}

std::future<Image> OpenCVFrameSource::implementation::post(int n){
	if (status() == concurrency::agent_status::agent_created) start();
	auto msg = std::make_shared<msg_type>(n, std::promise<Image>());
	auto future = msg->second.get_future();
	concurrency::send(mqueue, msg);
	return future;
}

void OpenCVFrameSource::implementation::run(){
	while (true)
	{
		std::shared_ptr<msg_type> msg = concurrency::receive(mqueue);
		try{
			msg->second.set_value(getFrame(msg->first));
		}
		catch (...){
			msg->second.set_exception(std::current_exception());
		}
	}
}
