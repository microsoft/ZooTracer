#include <ztKDTree.h>
#include <cassert>

using namespace zt;

KDTree::KDTree(
	Image frame,				// A video frame.
	const Projector& projector,	// Projector for the video.
	int pixel_step				// The number of pixels to step horizontally and vertically. Translates into the precision of nearest match coordinates.
	)
	: kd_ptr(new kd_tree_float()), features(new std::vector<float>()), step(pixel_step)
{
	int im_width = frame->width();
	int im_height = frame->height();
	int im_psize = static_cast<int>(frame->pixel_size());
	int patch_width = projector.patchWidth();
	int patch_height = projector.patchHeight();
	if ((projector.pixelSize() != im_psize) || (patch_width > im_width) || (patch_height > im_height))
		throw std::exception("cannot apply the projector to the image.");
	if (pixel_step < 1) throw std::exception("invalid argument: pixel_step.");

	//Image mask;

	//normalizedDifferenceImage(image, background, mask);
	//ditherImage(mask);
	// uchar * mask_data = mask.data();

	int n = projector.inputDim();
	int dimension = projector.outputDim();

	//PatchWindow patch(image, Coord(0, 0));

	//compute number of samples. 
	this->h_steps = (im_width - patch_width) / pixel_step; // patch_width + pixel_step * nw <= im_width
	int v_steps = (im_height - patch_height) / pixel_step;
	int point_count = h_steps*v_steps;
	features->resize(point_count * dimension);

	for (int iv = 0; iv < v_steps; iv++)
	for (int ih = 0; ih < h_steps; ih++){
		int i = ih + iv*h_steps;
		int x = ih*pixel_step;
		int y = iv*pixel_step;
		Image patch = frame->subImage(x, y, patch_width, patch_height); // no data copying
		//projector.project(patch, *features, i); // directly loading features to the vector
		auto f = projector.project(patch);
		std::copy(f.cbegin(), f.cend(), features->begin() + (i*dimension));
	}
	int max_per_leaf = 128; // the maximum number of nodes per leaf
	kd_ptr->build(dimension, point_count, features->data(), max_per_leaf);
}

KDTree::KDTree(std::string fileName)
: kd_ptr(new kd_tree_float()), features(new std::vector<float>())
{
	loadFromFile(fileName);
}


void KDTree::saveTo(FILE * target) const
{
	saveName(target);
	file_write(step, target);
	file_write(h_steps, target);
	kd_ptr->save(target);
}

void KDTree::loadFrom(FILE * source)
{
	checkName(source);
	file_read(step, source);
	file_read(h_steps, source);
	kd_ptr->load(source);
}

std::vector<Match> KDTree::getMatches(const std::vector<float>& descriptor, int num_matches, double approx_ratio) const
{
	kd_tree_float::neighbour_array nbrs;
	kd_ptr->get_neighbours(descriptor.data(), num_matches, nbrs, approx_ratio);

	std::vector<Match> output;

	int dim = kd_ptr->get_dimension();
	for (int i = 0; i < (int)nbrs.size(); ++i)
	{
		int idx = kd_ptr->get_indices()[nbrs[i].index];
		float* dp = kd_ptr->get_points() + nbrs[i].index*dim;
		std::vector<float> descriptor(dp, dp + dim); assert(descriptor.size() == dim);
		output.push_back(Match{ (idx % h_steps) * step, (idx / h_steps) * step, nbrs[i].distance, descriptor });
	}
	return output;
}