#pragma once

#include <cfloat>
#include <vector>
#include <memory>
#include <iosfwd>

// Forward-declaration of impl type, so clients do not need to see the template definitions.
template <class point_traits, bool with_scaling> struct kd_tree_impl;

// kd_tree class
// Keywords: kdtree, kd tree, k d tree, k-d tree, geometric indexing, geometric hashing, spatial index
// Documentation: http://codebox/kdtree

// NOTE: this version edited for use with AmpTrack. See accompanying txt file.

// Templated over a traits class modelling <c>kd_tree_traits</c>,
// and a bool flag <c>with_scaling</c> which signals whether scale factors
// are supplied to convert from the underlying point type (e.g. byte[16])
// to a wider type (e.g. double) when computing distances.
template <class point_traits, bool with_scaling = false>
class kd_tree {
public:
	static const bool scaling = with_scaling;

	/// The type of the elements of the raw points (e.g. byte, see <see>kd_tree_traits</see>)
	typedef typename point_traits::value_type value_type;

	/// The type of squared distances (e.g. uint, see <see>kd_tree_traits</see>)
	typedef typename point_traits::distance_type distance_type;

	/// The type of indices into the point array (e.g. uint32 see <see>kd_tree_traits</see>)
	typedef typename point_traits::index_type index_type;

	/// Structure used to return nearest neighbours
	struct neighbour {
		int index;
		distance_type distance;
	};

	/// Multiple neighbours are returned in a vector
	typedef std::vector<neighbour> neighbour_array;

	/// Construct an empty kd_tree.  
	kd_tree();

	/// Destructor.
	~kd_tree();

	/// Build a kd_tree.
	/// To allow for maximum memory efficiency, this method does not take a copy of the point set,
	/// and permutes the set in-place.  Repeat: this will permute your pointset!  Keep a copy if 
	/// you need it pristine.  To recover the permutation mapping the old to the new order, use
	/// <see>get_indices</see>.
	/// <param name="dim"/> Dimensionality of each point </param>
	/// <param name="npoints"/> Number of points </param>
	/// <param name="points">Points stored point-major, i.e. point1[0],...,point1[dim],point2[0],...,</param>
	/// <param name="max_per_leaf">Maximum number of points to store per leaf.  Values of 64-128 are generally good.</param>
	/// <param name="scaleConstant">Used only if the <c>with_scaling</c> template parameter is <c>true</c>. 
	///     Scale factors to multiply each dimension when computing distances. </param>
	void build(unsigned int dim, index_type npoints, value_type* points, unsigned int max_per_leaf = 64, double* scaleConstant = 0);

	/// [Approximate] [k-]nearest neighbour search.
	/// Return the <paramref name="K"/> nearest neighbours to <paramref name="query_point"/>.
	/// <param name="query_point">Pointer to query point, a <c>dim</c>-dimensional array of <c>value_type</c></param>
	/// <param name="K">Number of neighbours to return</param>
	/// <param name="neighbours">A <c>neighbour_array</c> to fill with the returned points.  
	///     It should be initialized to size <paramref name="K"/> before the call.
	/// </param>
	/// <param name="approxRatio">Set to greater than 1.0 for approximate search.  
	///        Distances are discounted by this factor when checking whether or not to backtrack.  
	///        A value of 0 will result in no backtracking.
	///        Sensible values to try are <c>pow(.1 to .8, dim)</c>.
	/// </param>
	void get_neighbours(value_type const* query_point, unsigned int K, neighbour_array& neighbours, double approxRatio = 1.0);

	/// Easy-to-use interface to <see>get_neighbours</see>
	neighbour_array get_neighbours(value_type const* query_point, unsigned int K, double approxRatio = 1.0);

	/// Print the tree to <param name="os"/>
	void print(std::ostream& os);

	/// Print statistics such as number of nodes explored etc.
	void print_stats(std::ostream& s);

	/// Reset statistics
	void reset_stats();


	/// Save to file <param name="filename"/>
	void save(std::string const& filename);

	/// Save to file <param name="file"/>
	void save(FILE * file);

	/// Load from file <param name="filename"/>
	void load(std::string const& filename);

	/// Load from file <param name="file"/>
	void load(FILE * file);

	/// Get our pointer to the points.  
	/// Remember that they are owned by the caller, unless the tree was loaded from file.
	value_type* get_points();

	/// Get the reordering indices.  
	/// If the input points array was P, then after 
	///   <code>
	///      typedef float Point[dim];
	///      vector<Point> P_reordered = copy(P);
	///      kd.build(..., P_reordered, ...);
	///      I = kd.get_indices();
	///   </code>
	///  the point at location k is the point that was at location I[k], or
	///   <code>
	///      P_reordered[k] = P[I[k]]
	///   </code>
	index_type* get_indices();

	/// Return the dimensionality of the points from which this tree was constructed
	int get_dimension() const;

	/// Return the number of points in this tree
	int get_npoints() const;

	/// Shorthand for the impl type.
	typedef kd_tree_impl<point_traits, with_scaling> impl_type;

	/// [Advanced] Get a pointer to the implementation.
	typename impl_type* get_impl() { return impl; }
private:
	impl_type* impl;
	kd_tree(const kd_tree& that) {}
};


// Traits classes are used to build various types of kd_tree.
// The key member is the <c>value_type</c>, which indicates the
// storage type of the input points (e.g. byte,float)
// You don't need to derive from this class, so this is a template
// in the normal (non-C++) sense: copy the class and edit the types
// appropriately.

struct kd_tree_traits {
	// value_type: Points in the tree are vectors of value_type[]
	typedef __int8 value_type;

	// diff_type: a type which can hold the difference of two value_types 
	typedef __int16 diff_type;

	// distance_type: a type which can store the distance between two points
	// If we think of 256 as an upper limit on the dimensionality of the points,
	// then this type needs 8 + 2*(B+1) bits, where B is the width of value_type.
	// E.g. for byte value_type, differences are 9 bits, squared diffs are 18 bits,
	// a sum of 256 squared diffs is 26 bits.  As this won't be stored, it 
	// can be any suitable native type larger than 26 bits, which we assume is true
	// for native unsigned int
	typedef unsigned int distance_type;

	// distance_max: A function which returns the largest possible distance_type
	static distance_type distance_max() { return (unsigned int)(-1); }

	// index_type: The type of indices into the points array.  
	// We generally choose this to be 32 bits, regardless of architecture,
	// as one will be stored per point, and for small dimensions (e.g. 16 
	// bytes), the extra 4 bytes per entry to store the index can be significant, 
	// so we force this to 32 bits.  If a limit of 4 billion points is not reasonable,
	// a given application might choose a 40 bit or 64 bit index type. (See 
	// traits for kd_byte_point_bigdata below).
	typedef unsigned __int32 index_type;

	// The allocator which will be used to get memory for the kdTree structures.
	// Note that allocation of the points themselves has already been done by the caller.
	typedef std::allocator<void> persistent_data_allocator;

	// A unique typetag used in save/load to warn about loading incompatible tree formats.
	static const int typetag = 0;
};

// Traits for byte points, with datasets smaller than 2^32 points
// See comments on <c>kd_tree_traits</c>
struct kd_byte_point {
	typedef unsigned char value_type;
	typedef double diff_type;
	typedef double distance_type;
	static distance_type distance_max() { return FLT_MAX; }
	typedef unsigned __int32 index_type;
	typedef std::allocator<void> persistent_data_allocator;
	static const int typetag = 1;
};

// Traits for byte points, with integer scaling.
// Datasets smaller than 2^32 points
// See comments on <c>kd_tree_traits</c>
struct kd_byte_point_integer_scaled {
	typedef unsigned char value_type;
	typedef int diff_type;
	typedef unsigned int distance_type;
	static distance_type distance_max() { return (unsigned int)(-1); }
	typedef unsigned __int32 index_type;
	typedef std::allocator<void> persistent_data_allocator;
	static const int typetag = 2;
};

// Traits for float points.
// Datasets smaller than 2^32 points
// See comments on <c>kd_tree_traits</c>
struct kd_float_point {
	typedef float value_type;
	typedef float diff_type;
	typedef double distance_type;
	static distance_type distance_max() { return FLT_MAX; }
	typedef unsigned __int32 index_type;
	typedef std::allocator<void> persistent_data_allocator;
	static const int typetag = 3;
};

// Traits for byte points.
// Datasets smaller than 2^64 points
// See comments on <c>kd_tree_traits</c>
struct kd_byte_point_bigdata {
	typedef float value_type;
	typedef float diff_type;
	typedef double distance_type;
	static distance_type distance_max() { return FLT_MAX; }
	typedef unsigned __int64 index_type;
	typedef std::allocator<void> persistent_data_allocator;
	static const int typetag = 4;
};


typedef kd_tree<kd_float_point> kd_tree_float;
typedef kd_tree<kd_byte_point> kd_tree_byte;
typedef kd_tree<kd_byte_point, true> kd_tree_byte_scaled;
// typedef kd_tree<kd_byte_point_integer_scaled, true> kd_tree_byte_int;
// typedef kd_tree<kd_byte_point_bigdata, true> kd_tree_byte_int;
