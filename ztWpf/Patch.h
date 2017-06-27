#pragma once

#include <ztPatch.h>

namespace ztWpf{
	public ref class Patch{
	public:
		Patch(int x, int y) : _x(x), _y(y){}
		Patch(const zt::Patch& p) : _x(p.x()), _y(p.y()){}
		property int x {int get(){ return _x; }; }
		property int y {int get(){ return _y; }; }
	private:
		int _x;
		int _y;
	};
}