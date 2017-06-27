#pragma once

#include <ztSaveable.h>
#include <ztImage.h>

#include <vector>

namespace zt
{

	// class for holding the means for dimensionality reduction 
	class Projector : public Saveable
	{
	private:
		std::vector<float> mean;
		std::vector<float> proj;
		std::vector<float> eigenvalues;
		std::vector<double> cov_sum; // TODO: could be float upto ~400,000 points
		int data_count;
		//int input_dim;
		int patch_width;
		int patch_height;
		size_t pixel_size;
		int output_dim;

		std::vector<float> weighting;


	public:
		Projector(
			int output_dimension,
			//int input_dimension,
			std::vector<Image> const & patches,
			bool Gaussian_weighting = true);

		Projector(std::string fileName);
		// prevent copying
		Projector(const Projector&) = delete;
		Projector& operator =(const Projector&) = delete;

		~Projector() {}

		std::vector<float> project(const Image&) const;

		// Load the feature vector directly to the array of features.
		//void project(const Image&, std::vector<float>& features, int index) const;

		Image reconstruct(const std::vector<float>&) const;

		int inputDim() const { return static_cast<int>(patch_width*patch_height*pixel_size); }

		// The number of principal components in the output space.
		int outputDim() const { return output_dim; }

		// Returns patch width in pixels.
		int patchWidth() const { return patch_width; }

		// Returns patch height in pixels.
		int patchHeight() const { return patch_height; }

		// Returns the size of each pixel in chars.
		int pixelSize() const { return static_cast<int>(pixel_size); }

		float get_eigenvalue(int i){ return eigenvalues[i]/data_count; }
		std::vector<float> get_proj_mat(){ return proj; }

		// static std::string extension; // cause windows "__dllonexit" initialization bug
		static std::string extension() { return ".proj"; }

		// Use data_count to tell whether the projector needs recalculating.
		bool ready(){ return data_count != 0; }

		// saveable implementation
		void saveTo(FILE *) const;
		void loadFrom(FILE *);
		std::string name() const { return "Projector"; }
	};

}