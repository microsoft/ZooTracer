//	Stand alone projection calculator.
//	Inputs via command line:
//		video file name.
//	    [output dimension] - default:  16
//		[patch size] - pixels on one size of square - default : 21
//		[number of samples] default: 10000 
//		[output file] - defaults to input file with extension replaced by .proj
//		[start frame number] = default:beginning of video
//		[end frame number] = default:end of video


//	Outputs.
//		Data file(s) adequate establish  a Projection from an arbitrary image patch
//		to an array of outputDimension floating point numbers.
//
//	Description.
//		calculate num_frames =~ sqrt(number of samples)
//		select num_frames frames, equally spaced between start and end frame.
//		for each such frame add num_frames random patches  from the frame into a training data set.
//		calculate the projection.	
//
// Ian Davies, 2013 , based on code take from Amptrack by A.Fitzgibbon.
// Vassily Lyutsarev, 2013, adaptation to zootracer library

// The below memory allocation checks require opencv adjustment
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include <string>
#include <cstdio>

#include <opencv/cv.h>

#include <OpenCVFrameSource.h>
#include <ztProjector.h>
#include <ztImage.h>

using namespace zt;
using namespace std;

const int defaultOutputDim = 16;
const int defaultPatchSize = 21;
const int defaultNumberOfSamples = 10000;

static char * videoFile = NULL;
static int outputDim;
static int patchSize;
static int numberOfSamples;
static int startFrame = -1;
static int endFrame = -1;
static string saveFile = "";

static int  usage()
{
	cout << "Usage GenereateProjector VideoFile [outputDimensions=16] [patch size=21] [num samples=10000] [start frame=1] [end frame=last frame] [saveFile] " << std::endl;
	return 1;
}


static bool parse_command_line(int argc, char** argv)
{

	if ((argc <= 1) || (argc >8))
		return false;
	videoFile = argv[1];

	outputDim = argc >2 ? atoi(argv[2]) : defaultOutputDim;

	patchSize = argc >3 ? atoi(argv[3]) : defaultPatchSize;
	if (outputDim <1 || outputDim >= 3 * patchSize*patchSize)
		return cerr << "Must have 1 <= outputDimensions < 3*patchSize^2" << std::endl, false;

	numberOfSamples = argc >4 ? atoi(argv[4]) : defaultNumberOfSamples;
	if (numberOfSamples <1)
		return cerr << "Must have 1 <= numberOfSamples" << std::endl, false;

	startFrame = argc >5 ? atoi(argv[5]) : startFrame;
	endFrame = argc >6 ? atoi(argv[6]) : endFrame;

	saveFile = argc >7 ? argv[7] : saveFile;
	if (saveFile.length() == 0)
	{
		string temp = videoFile;
		size_t last = temp.find_last_of('.');
		const string extension = ".projector";
		if (last != string::npos)
		{
			saveFile = temp.substr(0, last);
			saveFile += extension;
		}
		else
			saveFile = temp + extension;
	}
	return true;
}

static Image getRandomPatch(Image img, int patch_size)
{
	int x = rand() % (img->width() - 1 - patch_size);
	int y = rand() % (img->height() - 1 - patch_size);
	return img->subImage(x,y,patch_size,patch_size);
}

// check whether the environment variable is set to any value
bool detect_verbose(const char* env)
{
	size_t got;
	errno_t r = getenv_s(&got, NULL, 0, env);
	return (r == 0 || r == ERANGE) && got > 0;
}

void __stdcall verbose_progress(double v)
{
	printf("\r%2.1f %%", v * 100);
}

int main(int argc, char** argv)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(21007);
	bool verbose = detect_verbose("ZT_VERBOSE"); // check if AMPTRACK_VERBOSE is set in the run time environment
	if (!parse_command_line(argc, argv))
		return usage();

	OpenCVFrameSource vh(videoFile);
	if (vh.numFrames()>0)
	{
		startFrame = (startFrame <= 0 || startFrame > vh.numFrames()) ? 1 : startFrame;
		endFrame = (endFrame <= 0 || endFrame > vh.numFrames()) ? vh.numFrames() : endFrame;
		if (endFrame < startFrame)
		{
			cerr << "endFrame cannot be less than start frame" << std::endl;
			return 1;
		}
	}
	else
	{
		cerr << "Failed to open video file:" << videoFile << std::endl;
		return 1;
	}

	int nframesNeeded = int(sqrt(float(numberOfSamples)));
	int nStepBetweenFrames = max(1, (1 + endFrame - startFrame) / nframesNeeded);

	// calculate number of samples to take from each frame.
	int nActualFrames = 1 + (endFrame - startFrame) / nStepBetweenFrames;
	int nSamplesPerFrame = numberOfSamples / nActualFrames;
	int nSamplesTaken = 0;

	clock_t start_time = clock();
	if (verbose)
	{
		printf("reading file %s\nframes: %d-%d of %d  patch: %dx%d  dims: %d  samples: %d\n", videoFile, startFrame, endFrame, vh.numFrames(), patchSize, patchSize, outputDim, numberOfSamples);
	}

	vector<Image> td;
	for (int iFrame = startFrame; iFrame <= endFrame; iFrame += nStepBetweenFrames)
	{
		try{
			Image img = vh.getFrame(iFrame - 1);
			bool lastFrame = iFrame + nStepBetweenFrames > endFrame;

			int nSamplesThisFrame = lastFrame ? numberOfSamples - nSamplesTaken : nSamplesPerFrame;
			for (int nPatch = 1; nPatch <= nSamplesThisFrame; ++nPatch)
			{
				Image patch = getRandomPatch(img, patchSize);
				td.push_back(patch);
			}
			nSamplesTaken += nSamplesThisFrame;
		}
		catch (...){}
	}

	if (verbose)
	{
		printf("finished reading video file, elapsed %2.1f sec.\n", (double)(clock() - start_time) / CLOCKS_PER_SEC);
	}
	const int inputDimension = patchSize*patchSize * 3;  // = 21*21*3 1323 by default

	Projector projector(outputDim, td);

	if (verbose)
	{
		printf("finished pca, elapsed %2.1f sec.\n", (double)(clock() - start_time) / CLOCKS_PER_SEC);
	}
	// now save it..
	projector.saveToFile(saveFile);
	if (verbose)
	{
		printf("output file %s\nElapsed %2.1f sec.\n", saveFile.c_str(), (double)(clock() - start_time) / CLOCKS_PER_SEC);
	}

	return 0;
	// This is how to run the projector:
	// FeatureVector f_test = projector.project(td[0].patch);
	//printf("distance f_test to f_test		%8.3f\n", DistanceFunctions::hypSqrdDiff(f_brown_vertical,f_brown_vertical));
}


