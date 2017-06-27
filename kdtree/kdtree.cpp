//	Stand alone K-D tree builder.
//	Inputs via command line:
//		video file name.
//	    [projector file] - if not supplied use video file name with extension replaced by .projector
//		[start frame number] = default:beginning of video
//		[end frame number] = default:end of video
//		[output directory] - 	where the kdtree files (one per frame) will be stored. 
//							 	Defaults to sub-directory of the directory containing video file 
// 								called kdtrees.	
//	Outputs.
//		One file per frame which can then be used to build a kdtree.
//		to an array of outputDimension floating point numbers.
//
//	Description.
//		Build a projector from the given input file, 
//		Process the video, or each frame, 
//		project regularly spaced patches (pixel_skip apart), saving the resulting
//		lower dimensional summary in a kdtree.
//		Then write the kdtree to disk.
//
// Ian Davies, 2013 , based on code taken from Amptrack by A.Fitzgibbon.
// Vassily Lyutsarev, 2013, adaptation to zootracer library

// The below memory allocation checks require opencv adjustment
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include <OpenCVFrameSource.h>
#include <ztProjector.h>
#include <ztKDTree.h>

#include <opencv/cv.h>

#include <direct.h>
#include <iostream>
#include <cstdio>


using namespace zt;
using namespace std;

static string videoFile;
static string projectorFile;
static int startFrame = -1;
static int endFrame = -1;
static string saveDirectory = "";
// IGD - changed from 5 - because we are no longer using a random number between 1 and  pixel_skip
// when chosing distance between patches to go into the kdtree.
static int pixel_skip = 3;

static int usage()
{
	cerr << "Usage GenerateKDTrees VideoFile [projectorFile] [start frame] [end frame] [pixel skip=3] [saveDirectory] " << std::endl;
	return 1;
}




static bool parse_command_line(int argc, char** argv)
{

	if ((argc <= 1) || (argc >7))
		return false;
	videoFile = string(argv[1]);

	projectorFile = argc >2 ? argv[2] : projectorFile;

	if (projectorFile.length() == 0)
	{
		string temp = videoFile;
		size_t last = temp.find_last_of('.');
		const string extension = ".projector";
		if (last != string::npos)
		{
			projectorFile = temp.substr(0, last);
			projectorFile += extension;
		}
		else
			projectorFile = temp + extension;
	}

	startFrame = argc >3 ? atoi(argv[3]) : startFrame;
	endFrame = argc >4 ? atoi(argv[4]) : endFrame;

	if (argc >5)
	{
		pixel_skip = atoi(argv[5]);
		if (pixel_skip < 1)
			return cerr << "GenerateKDTrees pixel_skip must be at least 1" << std::endl, false;
	}

	if (argc >6)
		saveDirectory = argv[6];

	if (saveDirectory.length() == 0)
	{
		char* p = _fullpath(NULL, videoFile.c_str(), 0);
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath_s(p, drive, dir, fname, ext);
		free(p);
		strcat_s(dir, "kdtrees"); //_splitpath include trailing slash
		char path[_MAX_PATH];
		_makepath_s(path, drive, dir, NULL, NULL);
		saveDirectory = std::string(path);
		_mkdir(path);
	}

	return true;
}

int main(int argc, char** argv)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(21007);
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


	try{
		Projector proj(projectorFile);
		//for (int i = startFrame; i <= endFrame; i++)
		//{
		//	try {
		//		Image frame = vh.getFrame(i - 1);
		//		KDTree tree(frame, proj, pixel_skip);
		//		stringstream ss;
		//		ss << saveDirectory << (i - 1) << KDTree::extension();
		//		tree.saveToFile(ss.str());
		//	}
		//	catch (...){} // swallow all exceptions
		//}


		SimpleKDTreeSource k{ vh, proj, 3, 4 };
		getchar();
	}
	catch (const std::exception & e) {
		cerr << "loading projector: std::exception:" << e.what() << endl;
		return 1;
	}
	catch (...) {
		cerr << "loading projector:  other exception:" << endl;
		return 1;
	}
	getchar();
	return 0;
}


