
#pragma once

/// <summary>
/// An STL-like vector which can use internally allocated or externally supplied storage.
/// If the vector is using its own storage, it is in the "owned" state, and will free the
/// data on destruction.   
/// If it is pointing to externally supplied storage it is "attached", and the caller 
/// should manage the memory.
/// </summary>
/// 
template <class T>
class detachable_vector {
public:
	// Create "attached" vector, pointing to supplied data, which should be of size nmax.
	// In this case the caller should free the data.
	// Capacity and size are set to nmax
	detachable_vector(T* data_, size_t  nmax_):
	  data(data_),
	  sz(nmax_),
	  nmax(nmax_),
	  own_data(false)
	{
	}

	// Create "owned" vector, allocating space.
	// The vector will free the data.
	// Capacity and size are set to n
	detachable_vector(size_t  n_):
	  data(new T[n_]),
	  sz(n_),
	  nmax(n_),
	  own_data(true)
	{
	}

	// Create an empty vector, neither owned nor attached.
	detachable_vector():
	  data(0),
	  sz(0),
	  nmax(0),
	  own_data(false)
	{
	}

	// Copy that.
	// Owned state will be the same as before if this.capacity() >= that.size(),
	// otherwise state will become "owned".
	detachable_vector(const detachable_vector<T>& that):
	  data(new T[that.capacity()]),
	  sz(that.size()),
	  nmax(that.capacity()),
	  own_data(true)
	{
		copy(that.begin(), that.end(), data);
	}

    // Destroy this vector, freeing memory if "owned".
	~detachable_vector()
	{
		if (own_data)
			delete[] data;
	}

	// Use THAT as storage, becoming "attached"
	void attach(T* that, size_t that_size) {
	  if (own_data)
	    delete[] data;
	  
	  own_data = false;
	  data = that;
	  sz = nmax = that_size;
	}

	// Return our allocated data if "owned", null otherwise.
	// The data were allocated using new[], so should be freed using delete[].
	// If the data were not created here (i.e. we are "attached"), null is 
	// returned. In that case, the caller is expected to remember the pointer 
	// they attached.
	// The upshot is: if disown returns non-null, you must call delete[] after
	// this vector is destroyed.
	T* disown() {
		if (!own_data)
			return 0;
		own_data = false;
		return data;
	}

	// Ensure sufficient capacity to hold newmax entries.
	// This may reallocate, and therefore may change from "attached" to "owned".
	void reserve(size_t  newmax)
	{
		// No need to resize, own_data remains the same
		if (newmax <= nmax) 
			return;

		// Will resize, so will own the data.
		size_t min_expansion = nmax < 8 ? 8 : 3*nmax/2;
		if (newmax < min_expansion)
			nmax = min_expansion;
		else
			nmax = newmax;

		T* tmp = new T[nmax];
                T* tp = tmp;
		for(T const* p = data; p != data+sz; ++p)
                  *tp++ = *p;
		if (own_data)
			delete [] data;
		data = tmp;
		own_data = true;
	}

	// Ensure sufficient capacity to hold newmax entries and set size.
	// This may reallocate, and therefore may change from "attached" to "owned".
	void resize(size_t  newsize)
	{
		reserve(newsize);
		sz = newsize;
	}

	T& operator[](unsigned int i) {
		// ASSERT(i < sz);
		return data[i];
	}

	const T& operator[](unsigned int i) const {
		return data[i];
	}

	size_t size() const {
		return sz;
	}

	size_t capacity() const {
		return nmax;
	}

	T const* begin() const {
		return data;
	}

	T* begin() {
		return data;
	}
	
	T const* end() const {
		return data + sz;
	}

	T* end() {
		return data + sz;
	}

	void push_back(const T& t) {
		reserve(sz+1);
		data[sz] = t;
		++sz;
	}

private:
    T* data;
	size_t sz;   // Number of occupied elements.
	size_t nmax; // Capacity 
	bool own_data;
};


// Assign v[i] = value, having resized v if necessary
template <typename T>
inline void set_expanding_if_necessary(detachable_vector<T>& v, unsigned i, const T& value)
{
	if (i >= v.size()) {
		//if (i >= v.capacity()) std::cout << "kdt$" << std::flush;
		v.resize(i+1);
	}
	v[i] = value;
}
