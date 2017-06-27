
#define _SECURE_SCL 0

#include "kd_tree.h"

#define ASSERT(x) do { if (!(x)) throw new std::exception("ASSERTION FAILED: " #x); } while (false)

#include "kd_tree_impl.h"

// Instantiate template instances.  This generates the code
// for common versions of the kd_tree, so 
template struct kd_tree_impl<kd_byte_point, false>;
template struct kd_tree_impl<kd_byte_point, true>;
template struct kd_tree_impl<kd_float_point, false>;

template class kd_tree<kd_byte_point, false>;
template class kd_tree<kd_byte_point, true>;
template class kd_tree<kd_float_point, false>;

