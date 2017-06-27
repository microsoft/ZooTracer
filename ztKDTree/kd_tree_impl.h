
#include <cstdio>
#include <vector>
#include <stack>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <map>
#include <memory>
//#include <fstream>
#include <string>

#include "detachable_vector.h"
#include "array2d_adaptor.h"

#ifndef ASSERT
#define ASSERT(x)
#define AWF_DEFINED_ASSERT
#endif

template <class point_traits, bool with_scaling>
struct kd_tree_impl {
	typedef typename kd_tree<point_traits, with_scaling>::distance_type    distance_type;
	typedef typename kd_tree<point_traits, with_scaling>::value_type       value_type;
	typedef typename point_traits::diff_type  diff_type;

	typedef typename kd_tree<point_traits, with_scaling>::index_type index_type;
	typedef unsigned __int16 dimension_type;
	typedef __int32 signed_index_type;

	static const int typetag = point_traits::typetag;

	// This is a foursome of arrays rather than an array of 4-element structs 
	// in order to talk easily to matlab.
	
	// The splitting dimensions are stored as uint16
	detachable_vector<dimension_type> internalNodesSplitDim;

	// The splitting thresholds are stored as value_types
	detachable_vector<value_type> internalNodesSplitThreshold;

	// The indices to the left node are stored as int32.
	// Negative index indicates a leaf node, positive an internal node
	detachable_vector<signed_index_type> internalNodesLeft;

	// The indices to the right node are stored as int32.
	detachable_vector<signed_index_type> internalNodesRight;

	// Leaf node start indices.  
	// These index into leaf_node_point_indices (or into a reordered point array).
	// This vector has one dummy value at the start so that we can identify node types:
	//    internal: index >= 0 
	//        leaf: index <= -1
	detachable_vector<index_type> leafNodeTable;

	// Scale factors for with_scaling versions
	detachable_vector<double> invScaleConstant;

	// Total number of points.
	index_type npoints;

	// The indices for the input data
	detachable_vector <index_type> indices;

	// Either a pointer to the input points, or our own array, created at load time.
	detachable_vector<value_type> points;

	// Dimensionality; definitely below 2^32, so use native uint
	unsigned int d;

	// Rootnode: either -1 if the tree is all leaf, or 0
	signed_index_type rootnode;

	// stats
	unsigned int num_queries;
	unsigned int num_heap_actions;
	unsigned int num_leaves_explored;

	typedef typename kd_tree<point_traits, with_scaling>::neighbour_array neighbour_array;

	kd_tree_impl() {}

	void build(unsigned int dim, index_type npoints_in, value_type* points, unsigned int max_per_leaf, double* scaleConstant);
	void get_neighbours(value_type const* query_point, unsigned int num_neighbours, neighbour_array& neighbours, double approxRatio);
	void print(std::ostream& s, int node, int indent);
	void save(FILE * f);
	void load(FILE * f);

};
                                              

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree members
///////////////////
template<class point_traits, bool with_scaling>
kd_tree<point_traits, with_scaling>::kd_tree()
{
	impl = 0;
}

template<class point_traits, bool with_scaling>
kd_tree<point_traits, with_scaling>::~kd_tree()
{
	delete impl;
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::reset_stats()
{
	impl->num_queries = 0;
	impl->num_heap_actions = 0;
	impl->num_leaves_explored = 0;
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::print_stats(std::ostream& s)
{
	double v = 1.0 / impl->num_queries;
	s << "(per query: heap_actions=" << v*impl->num_heap_actions;
	s << ", explored=" << v*impl->num_leaves_explored;
	s << ")\n";
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::print(std::ostream& s)
{
	impl->print(s, impl->rootnode, 0);
}
                                                 
template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::build(unsigned int dim, index_type npoints, value_type* points, unsigned int max_per_leaf, double* scaleConstant)
{
	impl = new kd_tree_impl<point_traits, with_scaling>;
	reset_stats();
	impl->build(dim, npoints, points, max_per_leaf, scaleConstant);
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::get_neighbours(value_type const* query_point, unsigned int num_neighbours, neighbour_array& neighbours, double approxRatio)
{
	impl->get_neighbours(query_point, num_neighbours, neighbours, approxRatio);
}

template<class point_traits, bool with_scaling>
typename kd_tree<point_traits, with_scaling>::neighbour_array 
  kd_tree<point_traits, with_scaling>::get_neighbours(value_type const* query_point, unsigned int K, double approxRatio)
{
	neighbour_array neighbours;
	impl->get_neighbours(query_point, K, neighbours, approxRatio);
	return neighbours;
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::save(std::string const& filename)
{
	//std::ofstream f(filename, std::ios_base::binary | std::ios_base::out);
	FILE * f;
	errno_t err = fopen_s(&f, filename.c_str(), "wb");
	if (err) throw new std::exception(("cannot open file "+filename).c_str());
	save(f);
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::save(FILE * f)
{
	impl->save(f);
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::load(std::string const& filename)
{
	//std::cout << "Loading from " << filename << " ... " << std::flush;
	//std::ifstream f(filename, std::ios_base::binary);
	FILE * f;
	errno_t err = fopen_s(&f, filename.c_str(), "rb");
	if (err) throw new std::exception(("cannot open file " + filename).c_str());
	load(f);
}

template<class point_traits, bool with_scaling>
void kd_tree<point_traits, with_scaling>::load(FILE * f)
{
	delete impl;
	impl = new kd_tree_impl<point_traits, with_scaling>;
	reset_stats();
	impl->load(f);
}

template<class point_traits, bool with_scaling>
typename kd_tree<point_traits, with_scaling>::value_type* kd_tree<point_traits, with_scaling>::get_points()
{
	return impl->points.begin();
}

template<class point_traits, bool with_scaling>
typename kd_tree<point_traits, with_scaling>::index_type* kd_tree<point_traits, with_scaling>::get_indices()
{
	return impl->indices.begin();
}

template<class point_traits, bool with_scaling>
int kd_tree<point_traits, with_scaling>::get_dimension() const
{
	return impl->d;
}

template<class point_traits, bool with_scaling>
int kd_tree<point_traits, with_scaling>::get_npoints() const
{
	return impl->npoints;
}

//////////////////////////////////////////////////////////////////////////////////////////
// stack_element for building
///////////////////

struct kd_tree_build_stack_element {
	int direction;
	unsigned int parent_nodeindex;
	unsigned int range[2]; 
};

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_impl::build
///////////////////

template<class point_traits, bool with_scaling>
void kd_tree_impl<point_traits, with_scaling>::build(unsigned int dim, index_type npoints_in, value_type* points_in, unsigned int max_per_leaf, double* scaleConstant_ptr)
{
  d = dim;
  npoints = npoints_in;
  rootnode = -2;

  // Take a pointer to the data.
	array2d_adaptor<value_type> data(points_in, d);

	// And remember it for lookup
	this->points.attach(points_in, npoints*d);

	// Remember scale constants.
	if (with_scaling) {
		invScaleConstant.resize(d);
		for(unsigned i = 0; i < d; ++i)
			invScaleConstant[i] = 1.0/scaleConstant_ptr[i];
	}

	// Create initial indices
	// Leaf point indices
	//detachable_vector <index_type> indices(npoints); // Stick this at the top so that we can return it to matlab
	indices.resize(npoints);
	for(index_type i = 0; i < npoints; ++i)
		indices[i] = i;

	// The number of internal nodes is unknown in advance so store them as vectors.
	// In the worst possible case there will npoints - max_per_leaf internal nodes.
	// However in practice this will never happen. In the best case there are npoints/max_per_leaf - 1
	// internal nodes. Reserve 1.5x the best case amount of memory for the internal nodes.

	index_type nInternalNodesReserve = index_type(1.5 * npoints / max_per_leaf);
	internalNodesSplitDim.reserve(nInternalNodesReserve);
	internalNodesSplitThreshold.reserve(nInternalNodesReserve);
	internalNodesLeft.reserve(nInternalNodesReserve);
	internalNodesRight.reserve(nInternalNodesReserve);

	internalNodesSplitDim.resize(0);
	internalNodesSplitThreshold.resize(0);
	internalNodesLeft.resize(0);
	internalNodesRight.resize(0);

	// The number of leaf nodes are unknown in advance so the leafNodeTable is stored using
	// a vector. There are always n_internal_nodes+1 leaf nodes. 
	leafNodeTable.reserve(nInternalNodesReserve + 1);
	leafNodeTable.resize(1); // Make space for the unused node
	leafNodeTable[0] = -1;

	// Initialise a stack to contain the ranges
	std::stack <kd_tree_build_stack_element> rangeStack;

	// Create a stack element with all the leaf_point_indices. 
	kd_tree_build_stack_element allRange;
	// Initialise the direction to zero since the root node doesn't have a direction
	allRange.direction = 0;
	// Initialize the parent_nodeindex to zero
	allRange.parent_nodeindex = 0;
	// Initialise the range to the whole of the data
	allRange.range[0] = 0;
	allRange.range[1] = npoints - 1;

	// Initialise an empty array to store the variances in each dimension
	// detachable_vector<int> varDim(d, -1);

	// Push the stack element onto the stack
	rangeStack.push(allRange);

	// Initialise the counter variables
	index_type nodeIndex = 0; // next available internal node

	//////////////////////////////////// While the range stack is not empty ////////////////////////////////////
	while(!rangeStack.empty()) {
		// The number of points in this leaf node is the end of the range minus the beggining
		// of the range plus 1
		kd_tree_build_stack_element const& current = rangeStack.top();

		// Declare a variable to store the number of points per leaf node
		index_type nPointsThisNode = current.range[1] - current.range[0] + 1;

		ASSERT(nPointsThisNode > 0);

		//              std::cout << "range " << current.range[0] << ".." << current.range[1] << std::endl;

		////////////////////////////////////// If the current node is a leaf node ///////////////////////////////
		if (nPointsThisNode <= max_per_leaf) 
		{
			// Update the leaf node table
			leafNodeTable.push_back(current.range[0]);

			// And hook its location into its parent (negated, to signal it's a leaf)
			signed_index_type leafIndex = (signed_index_type)(leafNodeTable.size()-1);

			// If the leaf node is on the left
			if (current.direction == -1) {
				// Update the parent node's left index
				set_expanding_if_necessary(internalNodesLeft, current.parent_nodeindex, -leafIndex);

				// If the leaf node is on the right
			} else if (current.direction == 1) {
				// Update the internal nodes right index
				set_expanding_if_necessary(internalNodesRight, current.parent_nodeindex, -leafIndex);
			} else
				// Update the rootnode
				this->rootnode = -leafIndex;

			rangeStack.pop();
			//////////////////////////////// If the current node is an internal node ////////////////////////////////
		}
		else 
		{
			////////////////////////////// Update parent node's child pointer /////////////////////////////
			// If the node is on the left
			if (current.direction == -1) {
				// Update the internal nodes left index
				set_expanding_if_necessary(internalNodesLeft, current.parent_nodeindex, (signed_index_type)nodeIndex);
			}
			// If the node is on the right
			else if (current.direction == 1) {
				// Update the internal nodes right index
				set_expanding_if_necessary(internalNodesRight, current.parent_nodeindex, (signed_index_type)nodeIndex);
			} else
				// Update the rootnode
				this->rootnode = (signed_index_type)nodeIndex;

			/// Pointer to the subset of indices relating to the current node
			index_type* current_indices = &indices[0] + current.range[0];

			////////////////////////// Get ranges ///////////////////////////////////////////

			// Get dimension with the greatest range
			// We do this in double as it's not time-critical
			double maxDimRange=-0.1;
			int maxRangeDimension=-1;
			// For each dimension
			for(unsigned int cDim = 0;cDim < d;cDim++) {
				double maxValue = -FLT_MAX;
				double minValue = FLT_MAX;
				// For each data point in this node
				for(unsigned int cData = 0; cData < nPointsThisNode; cData ++) {
					// Update the max and min values for this dimension
					double v = data(cDim,current_indices[cData]);
					if (v > maxValue) maxValue = v;
					if (v < minValue) minValue = v;
				}
				// Compute the range for this dimension
				double dimRange = double(maxValue) - double(minValue);
				if (with_scaling) dimRange = dimRange * invScaleConstant[cDim];

				ASSERT(dimRange >= 0);

				// Update the maximum range and the dimension with the maximum range
				if (dimRange > maxDimRange) {
					maxDimRange = dimRange;
					maxRangeDimension = cDim;
				}
			}
			ASSERT(maxRangeDimension  != -1);

			// Update the internal nodes dimension
			set_expanding_if_necessary(internalNodesSplitDim, nodeIndex, (dimension_type)maxRangeDimension);

			/////////////////////// Sort indices //////////////////////////////
			// Sort the indices according to the data at maxDimRange
			struct mycompare {
				array2d_adaptor<value_type>& data;
				int row;

				mycompare(array2d_adaptor<value_type>& p_data, int p_row):
				data(p_data),
					row(p_row)
				{
				}

				bool operator ()(const unsigned __int32 a, const unsigned __int32 b) 
				{ 
					return data(row, a) < data(row, b);                                 
				}
			};                              
			mycompare cmp(data, maxRangeDimension);
			std::sort(current_indices, current_indices + nPointsThisNode, cmp);

			ASSERT(data(maxRangeDimension, current_indices[0]) <= data(maxRangeDimension, current_indices[1]));


			////////////////////// Find median /////////////////////////////////

			// Find the median

			// The mid point is the number of points in this node divded by 2 (floor if odd)
			int mid_point =  int(ceil((double)nPointsThisNode/2));

			// Declare a variable to store the splitting threshold
			double splitVal;

			// if the number of points in this node is odd
			if (nPointsThisNode % 2 == 1)
				// The median is the midpoint
				splitVal = data(maxRangeDimension,current_indices[mid_point-1]);
			// If the number of points in this node is even
			else
				// The median is the mean of the two middle points
				splitVal = (double)(data(maxRangeDimension, current_indices[mid_point-1])
				+ data(maxRangeDimension, current_indices[mid_point]))/2;

			// Update the internal nodes threshold value
			// matlab_single fsplitVal = (matlab_single)(splitVal * invScaleConstant[maxRangeDimension]);
			set_expanding_if_necessary(internalNodesSplitThreshold, nodeIndex, (value_type)splitVal);

			///////////////////// Pop and push ///////////////////////////////

			// Create a stack element for the ranges of the left data
			kd_tree_build_stack_element left;
			// Set the direction to -1 to indicate it is on the left
			left.direction = -1;
			// The parent_nodeindex is the current node index
			left.parent_nodeindex = nodeIndex;
			// The left range is the left part of the range for the current node
			left.range[0] = current.range[0];
			left.range[1] = current.range[0]+mid_point-1;
			ASSERT(left.range[0] <= left.range[1]);

			// Create a stack element for the ranges of the right data
			kd_tree_build_stack_element right;
			// Set the direction to 1 to indicate it is on the right
			right.direction = 1;
			// The parent_nodeindex is the current node index
			right.parent_nodeindex = nodeIndex;
			// The right range is the right part of the range for the current node
			right.range[0] = current.range[0]+mid_point;
			right.range[1] = current.range[1];
			ASSERT(right.range[0] <= right.range[1]);

			// Pop the stack
			rangeStack.pop();
			// Push the right element onto the stack
			rangeStack.push(right);
			// Push the left element onto the stack
			rangeStack.push(left);
			// Increment the node index
			nodeIndex++;

		}


	}

	// Permute the point set (inplace)
	{
		std::vector<value_type> rowtmp(npoints, value_type(0));
		for(unsigned j = 0; j < d; ++j) {         
			for(unsigned int i = 0; i < npoints; ++i)
				rowtmp[i] = data(j, indices[i]);
			for(unsigned int i = 0; i < npoints; ++i)
				data(j, i) = rowtmp[i];
		}
	}

	//        {
	//                std::vector<bool> visited(npoints, false);
	//                unsigned int i = 
	//  while (i != n) { // continue until all elements have been processed
	//    cycle_start = i;
	//    tmp = first[i];
	//    do { // walk around a cycle
	//      pi = p[i];
	//      visited[pi] = true;
	//      std::swap(tmp, first[pi]);
	//      i = pi;
	//    } while (i != cycle_start);
	//    
	//    // find the next cycle
	//    for (i = 0; i < n; ++i)
	//      if (visited[i] == false)
	//        break;
	//  }
	//}
	//
	//        for(int i = 0; 






}

//////////////////////////////////////////////////////////////////////////////////////////
// stack element for searching
///////////////////

struct search_stack_element {
	int node;
	double dist_to_plane;

	search_stack_element(int n, double d): node(n), dist_to_plane(d) {}
};

struct search_stack : public std::stack<search_stack_element>
{
	void pushnode(int n, double d) {
		push(search_stack_element(n, d));
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// heap element for searching
///////////////////

template <class point_traits>
struct kd_tree_heap_element {
	typename point_traits::distance_type distance;
	int index;

	kd_tree_heap_element()
	{
		distance = point_traits::distance_max();
		index = -1;
	}
};

template <class point_traits>
inline bool operator<(kd_tree_heap_element<point_traits> const& a, kd_tree_heap_element<point_traits> const& b)
{
	return a.distance < b.distance;
}


//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_compute_distance
///////////////////

template <class value_type, class distance_type, class diff_type>
inline distance_type kd_tree_compute_distance(const value_type* a, const value_type* b, unsigned int dim, distance_type dmax) 
{
	distance_type sumSqDifference = 0;

	// For each dimension in the datapoint
	for(unsigned int cDim = 0; cDim < dim;cDim++) {
		// Compute the difference between the current data-point and the 
		// querypoint at the current dimension
		diff_type difference = (diff_type)a[cDim] - (diff_type)b[cDim];

		// Square the difference
		distance_type sqDifference = difference*difference;

		// Update the sumSqDifference
		sumSqDifference += sqDifference;

		// If the sumSqDifference computed so far is greater
		// than the distance from the query point to the furthest
		// away point
		if (sumSqDifference >= dmax)
			break;
	}

	return sumSqDifference;
}

// Specialization for <unsigned char,int> using SSE2
template <>
inline unsigned int kd_tree_compute_distance<unsigned char, unsigned int, int>(const unsigned char* pa, const unsigned char* pb, unsigned int dim, unsigned int preemptTh) 
{
	int dist = 0;
	unsigned int d = 0;
	unsigned int D16 = (unsigned)dim & 15;
	if (D16)
	{
		for (/**/; d < D16; d++)
		{
			int diff = pa[d] - pb[d];
			dist += diff*diff;
		}
	}

#ifdef MSRC_KDTREE_USE_SSE2
	// awf: for 16-byte vectors this gave me 0 speedup?
	if( false ) // m_SupportSSE2 )
	{
		__m128i zero = _mm_setzero_si128();
		__m128i dist4 = zero;
		for (/**/; d < (unsigned)dim; d += 16)
		{
			// load 16 bytes
			__m128i m1 = _mm_loadu_si128((__m128i*)(pa+d));
			__m128i m2 = _mm_loadu_si128((__m128i*)(pb+d));

			__m128i mlo1 = _mm_unpacklo_epi8(m1, zero);
			__m128i mlo2 = _mm_unpacklo_epi8(m2, zero);
			__m128i diff8x16bit = _mm_sub_epi16(mlo1, mlo2);
			__m128i dist4x32bit = _mm_madd_epi16(diff8x16bit, diff8x16bit);
			dist4 = _mm_add_epi32(dist4, dist4x32bit);

			__m128i mhi1 = _mm_unpackhi_epi8(m1, zero);
			__m128i mhi2 = _mm_unpackhi_epi8(m2, zero);
			diff8x16bit = _mm_sub_epi16(mhi1, mhi2);
			dist4x32bit = _mm_madd_epi16(diff8x16bit, diff8x16bit);
			dist4 = _mm_add_epi32(dist4, dist4x32bit);
		}
		// add 4 intermediate sums together
		dist4 = _mm_add_epi32(dist4, _mm_srli_si128(dist4,4)); // dist4 += dist4 >> 32
		dist4 = _mm_add_epi32(dist4, _mm_srli_si128(dist4,8)); // dist4 += dist4 >> 64
		dist += _mm_cvtsi128_si32(dist4); // dist += dist4 & 0xFFFFFFFF
	}
	else
#endif
	{
		for (/**/; d < (unsigned)dim; d += 8)
		{
			for (int d1 = 0; d1 < 8; d1++)
			{
				int diff = pa[d+d1] - pb[d+d1];
				dist += diff*diff;
			}

			if(dist >= (int)preemptTh)
				break;
		}
	}

	return dist;
}

template <class value_type, class distance_type, class diff_type>
inline distance_type kd_tree_compute_distance_scaled(const value_type* a, const value_type* b, unsigned int dim, distance_type dmax, 
	double const* invScaleConstant) 
{
	distance_type sumSqDifference = 0;

	// For each dimension in the datapoint
	for(unsigned int cDim = 0; cDim < dim;cDim++) {
		// Compute the difference between the current data-point and the 
		// querypoint at the current dimension
		diff_type difference = (diff_type)a[cDim] - (diff_type)b[cDim];

		// correct for dequantization scale
		difference = (diff_type)(difference * invScaleConstant[cDim]);

		// Square the difference
		distance_type sqDifference = difference*difference;
		// Update the sumSqDifference
		sumSqDifference += sqDifference;
		// If the sumSqDifference computed so far is greater
		// than the distance from the query point to the furthest
		// away point
		if (sumSqDifference >= dmax) {
			// Only increment the bailout counter if all of the
			// dimensions are not explored
			//if (cDim < dim - 1) ++nBailoutArr;
			// Set the bailout flag to true
			goto done;
		}
	}
done:
	return sumSqDifference;
}

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_impl::get_neighbours
///////////////////

template <class point_traits, bool with_scaling>
void kd_tree_impl<point_traits, with_scaling>::get_neighbours (value_type const* queryPoint, 
	unsigned int num_neighbours, 
	neighbour_array& neighbours, 
	double approxRatio)
{
	++num_queries;

	unsigned int k = num_neighbours;
	neighbours.reserve(k);
	neighbours.resize(0);

	// Use an integer to store the number of bail outs
	unsigned int nBailoutArr = 0;

	///////////////////////////////////////// Declare other variable ////////////////////////////////////////

	// The stack that contains nodes to visit
	search_stack nodeStack;
	nodeStack.pushnode(rootnode, 0.0);

	// Store the current closest points as a vector
	// structs.
	/*sunflower::CSFHeap<kd_tree_heap_element<point_traits>> heap;
	heap.reserve(k+1);
	{
	kd_tree_heap_element<point_traits> he;
	he.distance = distance_t_max;
	heap.insert(he);
	}*/
	std::vector< kd_tree_heap_element<point_traits> > heap(k);
	// Make the closest Points vector into a heap
	make_heap(heap.begin(), heap.end());

	size_t nLeafNodes = leafNodeTable.size() - 1; // remember the dummy in front.
	ASSERT(nLeafNodes > 0);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	// While there are still nodes on the node stack
	while (!nodeStack.empty()) {
		// Get the index of the node at the top of the stack
		search_stack_element tos = nodeStack.top();
		int nodeIndex = tos.node;
		double dist_to_plane = tos.dist_to_plane;
		// Remove the node at the top of the stack
		nodeStack.pop();

		// Check whether we want to enter this node
		// If the distance from the query point to the hyper plane defined
		// by the current internal node is less than the distance from 
		// the query point to the furthest away point in the closest points heap.
		bool enter = dist_to_plane == 0 || (dist_to_plane < heap[0].distance*approxRatio);
		if (!enter) continue;

		///////////////////////////// If the current node is a leaf node ///////////////////////////////

		if (nodeIndex < 0) {
			// convert to index into leafNodeTable
			nodeIndex = -nodeIndex;

			// The index for the start of the leaf node is given by the entry
			// in the leaf node table
			int leafNodeStartIndex = leafNodeTable[nodeIndex];

			int leafNodeEndIndex;
			if ((unsigned int)nodeIndex+1 < leafNodeTable.size()) {
				// The index of the end of the leaf node is given 
				// by the index of the next leaf node in the leaf 
				// node table.
				leafNodeEndIndex = leafNodeTable[(unsigned int)nodeIndex+1] - 1;              
			} else {
				// The index of the end of the leaf node is equal to
				// the total number of entries in the leaf node
				leafNodeEndIndex = npoints - 1;
			}
			// The total number of points in this leaf node is given by the
			// index for the end minus the index for the start  
			int nDataThisLeaf = (leafNodeEndIndex - leafNodeStartIndex + 1);

			/////// Find the SSD from the query point to all the points in the leaf /////////

			// For each data point
			for(int cData = 0; cData < nDataThisLeaf; cData++) {
				// Pointer to the datapoint
				unsigned int point_index = leafNodeStartIndex + cData;
				value_type* point = points.begin() + point_index*d;

				distance_type dmax = heap[0].distance;

				distance_type dist = with_scaling ?
					kd_tree_compute_distance_scaled<value_type, distance_type, diff_type>(point, queryPoint, d, dmax, &invScaleConstant[0]) :
				kd_tree_compute_distance<value_type, distance_type, diff_type>(point, queryPoint, d, dmax);

				// If the point is closer..
				if (dist < dmax)
				{
					++num_heap_actions;

					//if (heap.size() >= k)
					//      heap.pop();

					//kd_tree_heap_element<point_traits> he;
					//he.distance = dist;
					//he.index = leafNodeStartIndex + cData;
					//heap.insert(he);

					// Put the element from the top of the heap to the back
					pop_heap(heap.begin(),heap.end());

					// Replace the element at the back with the current data point
					heap[k-1].distance = dist;
					heap[k-1].index = point_index;

					// Push the element at the back of the heap so that the heap
					// maintains its heap structure
					push_heap(heap.begin(), heap.end());

				}
			}
			++num_leaves_explored;

			///////////////////////////// If the current node is an internal node ///////////////////////////////

		} else {
			// Get the index of the internal node
			index_type internalNodeIndex = nodeIndex;
			// Get the squared error between the querypoint at the splitting dimension
			// and the splitting value for the current node
			dimension_type splitDim = internalNodesSplitDim[internalNodeIndex];
			value_type splitVal = internalNodesSplitThreshold[internalNodeIndex];
			signed_index_type leftIndex = internalNodesLeft[internalNodeIndex];
			signed_index_type rightIndex = internalNodesRight[internalNodeIndex];

			bool onLeft = queryPoint[splitDim] <= splitVal;
			double distToPlane = (double)((diff_type)queryPoint[splitDim] - (diff_type)splitVal);
			if (with_scaling) 
				distToPlane *= invScaleConstant[splitDim]; // correct for dequantization scaling
			distToPlane = distToPlane*distToPlane;

			// First push the further node
			if (onLeft) {
				nodeStack.pushnode(rightIndex, distToPlane);
				nodeStack.pushnode(leftIndex, 0);
			} else {
				nodeStack.pushnode(leftIndex, distToPlane);
				nodeStack.pushnode(rightIndex, 0);
			}
		}
	}

	// copy result into output array
	neighbours.resize(k);
	for(unsigned int cData = 0; cData < k;cData++) {
		neighbours[cData].distance = heap[cData].distance;
		neighbours[cData].index = heap[cData].index;
	}

	// and may as well sort it...
	struct cmp {
		bool operator()(kd_tree<point_traits, with_scaling>::neighbour const & a, kd_tree<point_traits, with_scaling>::neighbour const & b) {
			return a.distance < b.distance;
		}
	};
	sort(neighbours.begin(), neighbours.end(), cmp());


	// Write the stats to file
	/*FILE *fp;
	fp=fopen("andrewStats.txt", "w");
	char str[12]; 
	sprintf(str, "%d", num_queries);
	fprintf(fp, str);
	fprintf(fp, "\n");
	sprintf(str, "%d", num_heap_actions);
	fprintf(fp, str);
	fprintf(fp, "\n");
	sprintf(str, "%d", num_leaves_explored); 
	fprintf(fp, str);
	fclose (fp);*/

}

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_impl::print
///////////////////

template <class point_traits, bool with_scaling>
void kd_tree_impl<point_traits, with_scaling>::print(std::ostream& s, int node, int indent)
{
	// indent
	for(int i = 0; i < indent; ++i) s << " ";

	if (node < 0) {
		s << "leaf " << -node << ": start " << this->leafNodeTable[-node] << std::endl;
	} else {
		s << "x[" << this->internalNodesSplitDim[node] << "] < " << (double)this->internalNodesSplitThreshold[node] << std::endl;
		print(s, this->internalNodesLeft[node], indent+1);
		print(s, this->internalNodesRight[node], indent+1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_impl::save
///////////////////

std::exception* err(std::string const& s)
{
	return new std::exception(s.c_str());
}

static void skipword(FILE * s, std::string const & desired_word)
{
#define SKIPWORD_LIMIT 1024
	char word[SKIPWORD_LIMIT];
	int i = 0;
	char c = fgetc(s);
	while ( !( c==EOF || c==' ' || c=='\t' || c=='\n' ) && (i+1)<SKIPWORD_LIMIT )
	{
		word[i++] = c;
		c = fgetc(s);
	}
	word[i] = '\0';
	if ( strcmp(word,desired_word.c_str()) )
	{
		throw err("Wanted [" + desired_word + "], got [" + word + "]");
	}

#undef SKIPWORD_LIMIT
}

template <class T>
static void write(FILE * f, detachable_vector<T> const& v)
{
	// f.write(reinterpret_cast<const char *>(v.begin()), v.size() * sizeof v[0]);
	fwrite(v.begin(), sizeof(v[0]), v.size(), f);
}

template <class T>
static void read(FILE * f, detachable_vector<T>& v)
{
	// f.read(reinterpret_cast<char *>(v.begin()), v.size() * sizeof v[0]);
	fread(v.begin(), sizeof(v[0]), v.size(), f);
}

template <class point_traits, bool with_scaling>
void kd_tree_impl<point_traits, with_scaling>::save(FILE * f)
{
	struct sw // helper function for writing to oldskool file streams
	{
		static void fwrite_string(std::string const & s, FILE * f) {
			fwrite( s.c_str(), sizeof(char), s.length(), f);
		}
		static void fwrite_var(std::string const & s, void const * ptr, size_t size, size_t count, FILE * f) {
			fwrite_string(s, f);
			fwrite(ptr, size, count, f);
			fwrite_string("\n", f);
		}
		static void fwrite_int(std::string const & s, int const i, FILE * f) {
			fwrite_var(s, &i, sizeof(int), 1, f);
		}
		static void fwrite_uint(std::string const & s, unsigned int const i, FILE * f) {
			fwrite_var(s, &i, sizeof(unsigned int), 1, f);
		}
		static void fwrite_index_type(std::string const & s, index_type const i, FILE * f) {
			fwrite_var(s, &i, sizeof(index_type), 1, f);
		}
		static void fwrite_signed_index_type(std::string const & s, signed_index_type const i, FILE * f) {
			fwrite_var(s, &i, sizeof(signed_index_type), 1, f);
		}
	};

	//std::cout << "Saving to " << filename << " ... " << std::flush;
	
	sw::fwrite_string("kd_tree_binary_file\n", f);
	sw::fwrite_int("typetag ", typetag, f);
	sw::fwrite_uint("d ", d, f);
	sw::fwrite_index_type("n ", npoints, f);
	sw::fwrite_uint("nodes ", internalNodesSplitDim.size(), f);
	sw::fwrite_uint("leaves ", leafNodeTable.size(), f);
	sw::fwrite_signed_index_type("rootnode ", rootnode, f);
	write(f, internalNodesSplitDim);
	write(f, internalNodesSplitThreshold);
	write(f, internalNodesLeft);
	write(f, internalNodesRight);
	write(f, leafNodeTable);
	if (with_scaling)
		write(f, invScaleConstant);
	write(f, indices);
	fwrite(reinterpret_cast<char const*>(points.begin()), sizeof(points[0]), d * npoints, f);
	if (ferror(f)) throw err("kd_tree: failed to save");
}

//////////////////////////////////////////////////////////////////////////////////////////
// kd_tree_impl::load
///////////////////

template <class point_traits, bool with_scaling>
void kd_tree_impl<point_traits, with_scaling>::load(FILE * f)
{
	struct sr // helper function for reading from oldskool file streams
	{
		static void fread_var(std::string const & s, void * ptr, size_t size, size_t count, FILE * f) {
			skipword(f, s); 
			fread(ptr, size, count, f);
			fgetc(f); // skip newline
		}
		static void fread_int(std::string const & s, int & i, FILE * f) {
			fread_var(s, &i, sizeof(int), 1, f);
		}
		static void fread_uint(std::string const & s, unsigned int & i, FILE * f) {
			fread_var(s, &i, sizeof(unsigned int), 1, f);
		}
		static void fread_index_type(std::string const & s, index_type & i, FILE * f) {
			fread_var(s, &i, sizeof(index_type), 1, f);
		}
		static void fread_signed_index_type(std::string const & s, signed_index_type & i, FILE * f) {
			fread_var(s, &i, sizeof(signed_index_type), 1, f);
		}
	};

	// std::string line;
	// skipword(f, line);  if (line != "kd_tree_binary_file") throw err("bad header line : " + line);
	skipword(f, "kd_tree_binary_file");
	int typetag_read;
	sr::fread_int("typetag", typetag_read, f);
	if (typetag_read != typetag) throw err("bad typetag"); // TODO: better error reporting
	sr::fread_uint("d", d, f);
	sr::fread_index_type("n", npoints, f);
	int nodes;
	sr::fread_int("nodes", nodes, f);
	int leaves;
	sr::fread_int("leaves", leaves, f);
	sr::fread_signed_index_type("rootnode", rootnode, f);

	internalNodesSplitDim.resize(nodes);
	internalNodesSplitThreshold.resize(nodes);
	internalNodesLeft.resize(nodes);
	internalNodesRight.resize(nodes);
	leafNodeTable.resize(leaves);

	read(f, internalNodesSplitDim);
	read(f, internalNodesSplitThreshold);
	read(f, internalNodesLeft);
	read(f, internalNodesRight);
	read(f, leafNodeTable);
	if (with_scaling) {
		invScaleConstant.resize(d);
		read(f, invScaleConstant);
	}
	indices.resize(npoints);
	read(f, indices);
	points.resize(d * npoints);
	fread(points.begin(), sizeof(points[0]), d * npoints, f);
}
