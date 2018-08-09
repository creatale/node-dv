#ifndef __LSWMS_H__
#define __LSWMS_H__

enum {RET_OK, RET_ERROR};

#include "opencv2/core/core.hpp"
//#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "float.h"

#define PI_2 CV_PI/2

typedef std::vector<cv::Point> LSEG;
typedef struct _DIR_POINT
{
	cv::Point pt;
	float vx;
	float vy;

	_DIR_POINT(cv::Point _pt, float _vx, float _vy)
	{
		pt = _pt;
		vx = _vx;
		vy = _vy;
	};
	_DIR_POINT()
	{	
		pt = cv::Point(0,0);
		vx = 0;
		vy = 0;
	}
}DIR_POINT;

class LSWMS
{

private:
	//Verbose
	bool __verbose;	
	
	// Sizes
	cv::Size __imSize, __imPadSize;
	int __imWidth, __imHeight;

	// Window parameters	
	int __R, __N;

	// Line Segments	
	LSEG __lSeg;
	int __numMaxLSegs;

	// Images and maps
	cv::Mat __img, __imgPad;
	cv::Rect __roiRect;
	cv::Mat __imgPadROI;
	cv::Mat __G, __Gx, __Gy;	// Map of probability of line segments
	cv::Mat __M;			// Map of visited pixels
	cv::Mat __A;			// Map of angles

	cv::Mat __imAux;		// For debugging

	// Thresholds and variables
	int __meanG;
	std::vector<int> __sampleIterator;
	float __margin;

	// Mean-Shift central
	std::vector<cv::Point> __seeds;
	
	// Functions
	void setPaddingToZero(cv::Mat &img, int NN);
	void updateMask(cv::Point pt1, cv::Point pt2);

	// SOBEL-specific functions
	int computeGradientMaps(const cv::Mat &img, cv::Mat &G, cv::Mat &Gx, cv::Mat &Gy);

	// General functions
	int findLineSegments(const cv::Mat &G, const cv::Mat &Gx, const cv::Mat &Gy, cv::Mat &A, cv::Mat &M, std::vector<LSEG> &lSegs, std::vector<double> &errors);
	int lineSegmentGeneration(const DIR_POINT &dpOrig, LSEG &lSeg, float &error);

	// Growing and Mean-Shift
	float grow(const DIR_POINT &dpOrig, cv::Point &ptDst, int dir);
	int weightedMeanShift(const DIR_POINT &dpOrig, DIR_POINT &dpDst, const cv::Mat &M=cv::Mat());

public:
	LSWMS(const cv::Size imSize, const int R, const int numMaxLSegs=0, bool verbose=false);
	int run(const cv::Mat &img, std::vector<LSEG> &lSegs, std::vector<double> &errors);
	void drawLSegs(cv::Mat &img, std::vector<LSEG> &lSegs, cv::Scalar color=CV_RGB(255,0,0), int thickness=1);
	void drawLSegs(cv::Mat &img, std::vector<LSEG> &lSegs, std::vector<double> &errors, int thickness=1);

};

#endif // __LSWMS_H__
