#include <ztProjector.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <fstream>
#include <algorithm>

using namespace zt;

Projector::Projector(
	int output_dimension,
	std::vector<Image> const & patches,
	bool Gaussian_weighting)
	:
	output_dim(output_dimension),
	data_count(0)
{
	// perform parameter checks
	if (int(patches.size()) <= output_dim) throw std::invalid_argument("number of patches must exceed output dimension");
	patch_width = patches[0]->width();
	patch_height = patches[0]->height();
	pixel_size = patches[0]->pixel_size();
	int input_dimension = patch_width*patch_height*pixel_size;
	if ((0 > output_dimension) || (input_dimension < output_dimension)) throw std::invalid_argument("output_dimension");
	if (Gaussian_weighting && (patch_width != patch_height)) throw std::exception("patches must be square when using Gaussian weighting");
	mean.resize(input_dimension);
	cov_sum.resize(input_dimension*input_dimension, 0);
	proj.resize(input_dimension*output_dimension);
	weighting.resize(input_dimension, 1);
	// compute weighting if necessary
	if (Gaussian_weighting) {
		// generate gaussian matrix
		// patches must be square
		int size = patch_width; // (int)std::sqrt(input_dimension / 3.0);
		double sigma = size / 2.7;

		// find filter centre
		double midpoint = (size - 1.0) / 2.0;

		// exponent constants
		double s = 1.0 / (2.0 * sigma * sigma);

		// generate gaussian values
		std::vector<double> gauss(size);
		for (int i = 0; i < size; ++i)
		{
			double x = i - midpoint;
			gauss[i] = exp(-x*x * s);
		}

		// multiply
		int ii = 0;
		for (int y = 0; y < size; ++y)
		for (int x = 0; x < size; ++x)
		for (int c = 0; c < pixel_size; ++c, ++ii)
			weighting[ii] = (float)(gauss[x] * gauss[y]);

	}
	// calculate mean and covariates
	std::vector<double> mean_sum(input_dimension, 0); // one elt for pixel RGB value in a patch.

	this->data_count = patches.size();      // number of exemplars.
	int n = input_dimension;

	// accumulate mean and covariance sums
	for (int i = 0; i < data_count; ++i)       // for each patch.
	{
		//if (patch_width != patches[i]->width()
		//	|| patch_height != patches[i]->height()
		//	|| pixel_size != patches[i]->pixel_size()) throw std::exception("incomapible patch!");
		auto pd = patches[i]->image_data();
		if (pd.size() != input_dimension) throw std::exception("incomapible patch!");
		uchar const * pd_ii = pd.data();
		double * ms = mean_sum.data();
		double * cs = cov_sum.data();
		float const * wd_ii = weighting.data();

		for (int ii = 0; ii < n; ++ii, ++ms, ++pd_ii, ++wd_ii) // iterate over each position in patch.
		{
			double v = (*pd_ii) * (*wd_ii);
			*ms += v;                      // acumulate (weighted) values at this position in patch, over all patches.
			uchar const * pd_jj = pd.data();
			float const * wd_jj = weighting.data();
			// TODO: int jj==ii then cv::completeSymm()
			for (int jj = 0; jj < n; ++jj, ++cs, ++pd_jj, ++wd_jj) // iterate over 
			{
				double u = (*pd_jj) * (*wd_jj);
				*cs += v * u;
			}
		}
	}

	// pointer into mean data
	float * md = mean.data();

	// calculate mean
	double * ms_i = mean_sum.data();
	for (int i = 0; i < input_dimension; ++i, ++md, ++ms_i)
	{
		*md = (float)(*ms_i / data_count);
	}

	// calculate covariance
	double * csd = cov_sum.data();
	float * m_ii = mean.data();

	n = input_dimension;

	cv::Mat cov(n, n, CV_32FC1);
	float * covd = reinterpret_cast<float*>(cov.data);

	for (int ii = 0; ii < n; ++ii, ++m_ii)
	{
		float * m_jj = mean.data();
		for (int jj = 0; jj < n; ++jj, ++covd, ++csd, ++m_jj)
		{
			*covd = (float)(*csd - data_count * (*m_ii) * (*m_jj));
		}
	}

	//clock_t time1 = getClockTick();

	// calculate top 'dim' eigenvalues
	int dim = this->output_dim;
	cv::Mat evals;
	cv::Mat evects;
	cv::eigen(cov, evals, evects, 0, dim - 1);


	// TODO: compare to cv::PCA and cv::SVD

	// pointer into projection data
	float * pd = proj.data();

	// copy eigen vectors into projection matrix
	for (int j = 0; j < dim; ++j)
	{
		//float * rd = reinterpret_cast<float*>(evects.row(j).data);
		for (int i = 0; i < n; ++i)
		{
			*pd++ = evects.at<float>(j, i);
		}
		eigenvalues.push_back(evals.at<float>(j));
	}
}


//std::vector<float> Projector::project(const Image& patch) const
//{
//	std::vector<float> output(output_dim);
//	project(patch, output, 0);
//	return output;
//}

std::vector<float> Projector::project(const Image& patch) const
{
	int input_dim = inputDim();
	std::vector<float> output(output_dim, 0.0f);
	//project(patch, output, 0);
	auto data = patch->image_data();
	if (data.size() != input_dim) throw std::invalid_argument("patch");
	for (int i = 0; i < output_dim; ++i)
	{
		for (int j = 0; j < input_dim; ++j) output[i] += proj[input_dim*i + j] * (data[j] * weighting[j] - mean[j]);
	}
	return output;
}

//void Projector::project(const Image& patch, std::vector<float>& features, int index) const
//{
//	if ((patch->width() != patch_width) || (patch->height() != patch_height) || (patch->pixel_size() != pixel_size))
//		throw std::invalid_argument("patch");
//	if ((index<0) || (output_dim * (index + 1) > int(features.size())))
//		throw std::out_of_range("index");
//	int input_dim = inputDim();
//	float * od_j = features.data() + index*output_dim;
//	float const * pj_ii = proj.data();
//	for (int j = 0; j < output_dim; ++j, ++od_j)
//	{
//		*od_j = 0;
//		float const * wd_i = weighting.data();
//		float const * md_i = mean.data();
//		for (int irow = 0; irow < patch_height; irow++)
//		{
//			uchar const * pd_i = patch->data(irow);
//			// multiply up one projection vector at a time...
//			for (int i = 0; i < patch_width; ++i, ++wd_i, ++md_i, ++pd_i, ++pj_ii)
//			{
//				*od_j += (*pj_ii) * ((*pd_i)*(*wd_i) - *md_i);
//			}
//		}
//	}
//
//}

Image Projector::reconstruct(const std::vector<float>& descr) const{
	if(static_cast<int>(descr.size())!=output_dim) throw std::out_of_range("descr");
	int input_dim = inputDim();
	std::vector<float> d = mean;
	std::vector<uchar> img(input_dim);
	for (int i = 0; i < output_dim; i++)
	for (int j = 0; j < input_dim; j++) d[j] += descr[i] * proj[input_dim*i + j];
	//float mi = d[0];
	//float ma = d[0];
	//for (int j = 1; j < input_dim; j++){
	//	mi = std::min(mi, d[j]);
	//	ma = std::max(ma, d[j]);
	//}
	//for (int j = 0; j < input_dim; j++) img[j] = uchar((d[j] - mi) * 255 / (ma - mi));
	for (int j = 0; j < input_dim; j++) img[j] = static_cast<uchar>(std::min(255.0f, std::max(0.0f, d[j]/weighting[j]+0.5f)));
	return make_image(patch_width, patch_height, pixel_size, img);
}

Projector::Projector(std::string fileName)
{
	loadFromFile(fileName);
}

void Projector::saveTo(FILE * target) const
{
	saveName(target);

	file_write(mean, target);
	file_write(proj, target);
	file_write(cov_sum, target);
	file_write(weighting, target);
	file_write(data_count, target);
	file_write(patch_width, target);
	file_write(patch_height, target);
	file_write(static_cast<int>(pixel_size), target);
	file_write(output_dim, target);
	file_write(eigenvalues, target);
}

void Projector::loadFrom(FILE * source)
{
	checkName(source);

	file_read(mean, source);
	file_read(proj, source);
	file_read(cov_sum, source);
	file_read(weighting, source);
	file_read(data_count, source);
	file_read(patch_width, source);
	file_read(patch_height, source);
	int size;
	file_read(size, source);
	pixel_size = static_cast<size_t>(size);
	file_read(output_dim, source);
	file_read(eigenvalues, source);}