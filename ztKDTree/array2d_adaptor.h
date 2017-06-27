
#pragma once


/// <summary>
/// Address a column-major matrix by row and column.
/// Does not copy the matrix, just takes a pointer.
/// </summary>
/// 
template <class T>
class array2d_adaptor {
	T* array1d;
	int colsize;
public:
	array2d_adaptor(T* array1d, int colsize) {
		this->array1d = array1d;
		this->colsize = colsize;
	}

	T& operator()(int row, int column) const {
		return array1d[column * colsize + row];
	}

	T* ptr() const { return array1d; }
};
