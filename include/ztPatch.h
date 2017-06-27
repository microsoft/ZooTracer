#pragma once

namespace zt{

	using FrameIndex = int;

	// A pair of offsets from left and top adges of an image.
	class Patch{
	private:
		int _x;
		int _y;
	public:
		Patch(int x, int y) : _x(x), _y(y){}

		// An offset from the left edge of an image.
		int x() const { return _x; }

		// An offset from the top edge of an image.
		int y() const { return _y; }
	};
}