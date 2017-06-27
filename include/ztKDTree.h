#pragma once

#include <ztSaveable.h>
#include <ztImage.h>
#include <ztPatch.h>
#include <ztProjector.h>
#include <OpenCVFrameSource.h>

#include <kd_tree.h>
#include <memory>
#include <string>

namespace zt{

	// Appearance descriptor a.k.a. filter jet.
	using Descriptor = std::vector<float>;

	// Patch offset (x,y) and  dissimilarity with the pattern
	using Match = std::tuple<int, int, double, std::vector<float>>;

	// Holds a search tree for a video frame. 
	class KDTree : public Saveable
	{
	private:
		std::unique_ptr<kd_tree_float> kd_ptr;
		std::unique_ptr<std::vector<float>> features; // when built from data kd will point to data in the vector.
		int h_steps = 0;	// The number of steps in horizontal direction.
		int step;		// The number of pixels to step horizontally and vertically.


	public:
		KDTree(
			Image frame,				// A video frame.
			const Projector& projector,	// Projector for the video.
			int pixel_step				// The number of pixels to step horizontally and vertically. Translates into the precision of nearest match coordinates.
			);
		KDTree(std::string fileName);
		~KDTree(){}

		std::vector<Match> getMatches(
			const std::vector<float>& descriptor,	// The pattern to match
			int num_matches,						// The number of nearest matches to find
			double approx_ratio = 0.3				// An approximation Ratio of 1.0 finds the exact nearst neighbours. Lower values are less accurate but faster.
			) const;

		std::string name() const { return "KDTree"; }
		void saveTo(FILE *) const;
		void loadFrom(FILE *);
		static std::string extension() { return ".kdt"; }
	};

	// An abstraction to somehow retrieve a tree related to a particular video frame.
	// The trees may become available upon the object construction or produced later in a background job.
	// You cannot copy a KDTreeSource. Use references or shared pointers instead.
	class KDTreeSource{
	protected:
		KDTreeSource(){}
	public:
		virtual ~KDTreeSource() {}
		KDTreeSource(const KDTreeSource&) = delete;
		KDTreeSource& operator = (const KDTreeSource&) = delete;

		// Synchronously get the tree. May block the thread until the tree is ready.
		virtual std::shared_ptr<KDTree> operator [] (FrameIndex) const = 0;

		// Asynchronously check if the tree is ready.
		virtual bool is_ready(FrameIndex) const = 0;

		using ProgressHandler = void(__stdcall *)(FrameIndex);
		// Subscribe progress notifications. the handler replaces current subscription and can be a nullptr to cancel notifications.
		virtual void subscribe(ProgressHandler) = 0;

		virtual int num_frames() const = 0;
	};

	// an implementation that generates trees in background threads, saves/loads them to files and keeps the trees in memory.
	class FileKDTreeSource :
		public zt::KDTreeSource
	{
	public:
		FileKDTreeSource(OpenCVFrameSource& video, const Projector& projector, int pixel_step, int number_of_workers, std::string folder_path);
		~FileKDTreeSource() override;

		// Synchronously get the tree. May block the thread until the tree is ready.
		std::shared_ptr<KDTree> operator [] (FrameIndex) const override;

		// Asynchronously check if the tree is ready.
		bool is_ready(FrameIndex) const override;

		// Subscribe progress notifications. the handler replaces current subscription and can be a nullptr to cancel notifications.
		void subscribe(ProgressHandler) override;

		int num_frames() const override;
	private:
		class implementation;
		std::unique_ptr<implementation> impl;
	};

	// an implementation that generates trees in background threads and keeps the trees in memory.
	class SimpleKDTreeSource :
		public zt::KDTreeSource
	{
	public:
		SimpleKDTreeSource(OpenCVFrameSource& video, const Projector& projector, int pixel_step, int number_of_workers);
		~SimpleKDTreeSource() override;

		// Synchronously get the tree. May block the thread until the tree is ready.
		std::shared_ptr<KDTree> operator [] (FrameIndex) const override;

		// Asynchronously check if the tree is ready.
		bool is_ready(FrameIndex) const override;

		// Subscribe progress notifications. the handler replaces current subscription and can be a nullptr to cancel notifications.
		void subscribe(ProgressHandler) override;

		int num_frames() const override;
	private:
		class implementation;
		std::unique_ptr<implementation> impl;
	};



}