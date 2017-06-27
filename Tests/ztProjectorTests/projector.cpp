#include "stdafx.h"
#include "CppUnitTest.h"

#include <ztProjector.h>
#include <opencv2/core/core.hpp>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ztProjectorTests
{

	using std::vector;
	using zt::Image;

	TEST_CLASS(projector)
	{
	public:

		TEST_METHOD(projector_pipeline)
		{
			// 1. Create 4 random "base images" all 2x2x3 = 12 bytes long
			const int dim = 4;
			const int w = 2;
			const int h = 2;
			const int c = 3;
			const int whc = w*h*c;
			vector<vector<unsigned char>> base;
			for (int i = 0; i < dim; i++){
				vector<unsigned char> b(whc);
				for (auto& i : b) i = static_cast<unsigned char>(rand() % 8);
				base.push_back(b);
			}

			// 2. Create 24 linear combinations
			const int n = 24;
			vector<Image> patches;
			for (int p = 0; p < n; ++p){
				vector<unsigned char> v(dim);
				for (auto& i : v) i = static_cast<unsigned char>(rand() % 8);
				vector<unsigned char> img(whc);
				for (int j = 0; j < 12; j++)
				{
					unsigned char pixel = 0;
					for (int i = 0; i < dim; i++)
						pixel += v[i] * base[i][j]; // v<8, base<8 => dim*v*base < 4*8*8 = 256
					img[j] = pixel;
				}
				auto image = zt::make_image(w, h, c, img);
				patches.push_back(image);
			}

			// 3. Create a projector;
			zt::Projector proj{ dim, patches, false };
			Assert::AreEqual(dim, proj.outputDim());
			Assert::AreEqual(whc, proj.inputDim());

			// 4. Check the projection matrix is orthonormal
			auto mat = proj.get_proj_mat();
			Assert::AreEqual(size_t(dim*whc), mat.size());
			for (int i = 0; i < dim; ++i)
			{
				for (int j = 0; j < dim; ++j){
					float sum = 0;
					for (int k = 0; k < whc; ++k) sum += mat[i * whc + k] * mat[j * whc + k];
					Assert::AreEqual(i == j ? 1.0f : 0.0f, sum, 1e-5f);
				}
			}
			// 4. all the patches have to be projected/reconstructed exactly
			for (auto& p : patches){
				auto d = proj.project(p);
				auto r = proj.reconstruct(d);
				auto d1 = p->image_data();
				auto d2 = r->image_data();
				Assert::IsTrue(d1 == d2);
			}
		}

		TEST_METHOD(projector_pipeline_weights)
		{
			// 1. Create 4 random "base images" all 2x2x3 = 12 bytes long
			const int dim = 4;
			const int w = 2;
			const int h = 2;
			const int c = 3;
			const int whc = w*h*c;
			vector<vector<unsigned char>> base;
			for (int i = 0; i < dim; i++){
				vector<unsigned char> b(whc);
				for (auto& i : b) i = static_cast<unsigned char>(rand() % 8);
				base.push_back(b);
			}

			// 2. Create 24 linear combinations
			const int n = 24;
			vector<Image> patches;
			for (int p = 0; p < n; ++p){
				vector<unsigned char> v(dim);
				for (auto& i : v) i = static_cast<unsigned char>(rand() % 8);
				vector<unsigned char> img(whc);
				for (int j = 0; j < 12; j++)
				{
					unsigned char pixel = 0;
					for (int i = 0; i < dim; i++)
						pixel += v[i] * base[i][j]; // v<8, base<8 => dim*v*base < 4*8*8 = 256
					img[j] = pixel;
				}
				auto image = zt::make_image(w, h, c, img);
				patches.push_back(image);
			}

			// 3. Create a projector;
			zt::Projector proj{ dim, patches, true };
			Assert::AreEqual(dim, proj.outputDim());
			Assert::AreEqual(whc, proj.inputDim());

			// 4. Check the projection matrix is orthonormal
			auto mat = proj.get_proj_mat();
			Assert::AreEqual(size_t(dim*whc), mat.size());
			for (int i = 0; i < dim; ++i)
			{
				for (int j = 0; j < dim; ++j){
					float sum = 0;
					for (int k = 0; k < whc; ++k) sum += mat[i * whc + k] * mat[j * whc + k];
					Assert::AreEqual(i == j ? 1.0f : 0.0f, sum, 1e-5f);
				}
			}
			// 4. all the patches have to be projected/reconstructed exactly
			for (auto& p : patches){
				auto d = proj.project(p);
				auto r = proj.reconstruct(d);
				auto d1 = p->image_data();
				auto d2 = r->image_data();
				Assert::IsTrue(d1 == d2);
			}
		}
		TEST_METHOD(image_data_continuous)
		{
			std::vector<unsigned char> d{
			10, 20, 30,  40, 50, 60,  70, 80, 90,  100, 110, 120,
			11, 21, 31,  41, 51, 61,  71, 81, 91,  101, 111, 121,
			12, 22, 32,  42, 52, 62,  72, 82, 92,  102, 112, 122,
			13, 23, 33,  43, 53, 63,  73, 83, 93,  103, 113, 123
		};
			auto img = zt::make_image(4, 4, 3, d);
			auto d_img = img->image_data();
			Assert::IsTrue(d == d_img);

			auto d1 = img->subImage(0, 0, 1, 1)->image_data();
			Assert::IsTrue(std::vector<unsigned char>{
				10, 20, 30
			} == d1);

			auto d2 = img->subImage(1, 1, 3, 2)->image_data();
			Assert::IsTrue(std::vector<unsigned char>{
				41, 51, 61,  71, 81, 91,  101, 111, 121,
				42, 52, 62,  72, 82, 92,  102, 112, 122
			} == d2);
		}

	};
}