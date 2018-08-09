#include "LSWMS.h"

#ifdef linux
	#include <stdio.h>
#endif

#include <iostream>
#include <stdlib.h>

#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#define NOT_A_VALID_ANGLE 5
#define ANGLE_MARGIN	22.5
#define MAX_ERROR	0.19625 // ((22.5/2)*CV_PI/180

using namespace cv;
using namespace std;

static void setTo14Quads(DIR_POINT &dp)
{
	if(dp.vx < 0)
	{	
		dp.vx = -dp.vx;
		dp.vy = -dp.vy;
	}
}

LSWMS::LSWMS(const cv::Size imSize, const int R, const int numMaxLSegs, bool verbose)
{
	// **********************************************
	// Constructor of class LSWMS (Slice Sampling Weighted
	// Mean-Shift)
	// Args:
	// 	-> imSize - Size of image
	//	-> R - accuracy parameter
	// 	-> numMaxLSegs - requested number of line segments.
	//			if set to 0, the algorithm finds exploring
	//			the whole image until no more line segments
	// 			can be found
	//	-> verbose - show messages
	// **********************************************

	__verbose = verbose;

	// Init variables
	__imSize = imSize;
	__imWidth = imSize.width;
	__imHeight = imSize.height;
	
	__R = R;
	__numMaxLSegs = numMaxLSegs;

	__N = 2*__R + 1;

	// Add padding it necessary
	if( (__imSize.width + 2*__N) % 4 != 0)	
		__N = __N + ((__imSize.width + 2*__N) % 4)/2;		
	
	__imPadSize.width = __imSize.width + 2*__N;
	__imPadSize.height = __imSize.height + 2*__N;	

	// Init images
	__img = cv::Mat(__imSize, CV_8U);
	__imgPad = cv::Mat(__imPadSize, CV_8U);
	__roiRect = cv::Rect(__N, __N, __imSize.width, __imSize.height);

	// Mask image
	__M = cv::Mat(__imPadSize, CV_8U);
	__M.setTo(255);

	// Angle mask
	__A = cv::Mat(__imPadSize, CV_32F);
	__A.setTo(NOT_A_VALID_ANGLE);
	
	// Gradient images
	__G = cv::Mat(__imPadSize, CV_8U);
	__G.setTo(0);
	__Gx = cv::Mat(__imPadSize, CV_16S);
	__Gx.setTo(0);
	__Gy = cv::Mat(__imPadSize, CV_16S);
	__Gy.setTo(0);

	// Iterator
	if(__numMaxLSegs != 0)
	{
		__sampleIterator = std::vector<int>(__imSize.width*__imSize.height, 0);
		for(unsigned int k=0; k<__sampleIterator.size(); k++)
			__sampleIterator[k] = k;

		cv::randShuffle(__sampleIterator);			
	}
	
	// Angular m_margin
	__margin = (float)(ANGLE_MARGIN*CV_PI/180);
}

int LSWMS::run(const cv::Mat &img, std::vector<LSEG> &lSegs, std::vector<double> &errors)
{
	// **********************************************
	// This function analyses the input image and finds
	// line segments that are stored in the given vector
	// Args:
	// 	-> img - Color or grayscale input image
	// 	<- lSegs - Output vector of line segments
	//  <- errors - Output vector of angular errors
	// Ret:
	// 	RET_OK - no errors found
	// 	RET_ERROR - errors found
	// **********************************************

	// Clear line segment container
	lSegs.clear();
	errors.clear();

	// Input image to __img
	if(img.channels() == 3)
		cv::cvtColor(img, __img, CV_BGR2GRAY);
	else
		__img = img;

	// Add convolution borders
	cv::copyMakeBorder(__img, __imgPad, __N, __N, __N, __N, cv::BORDER_REPLICATE); // This way we avoid line segments at the boundaries of the image

	// Init Mask matrix
	__M.setTo(255);
	__imgPadROI = __M(__roiRect);
	__imgPadROI.setTo(0);

	// Compute Gradient map		
	// Call to the computation of the gradient and angle maps (SOBEL)
	int retP = computeGradientMaps(__imgPad, __G, __Gx, __Gy);
	if(retP == RET_ERROR)
	{
		if(__verbose) {	printf("ERROR: Probability map could not be computed\n"); }
		return RET_ERROR;
	}	

	// Set padding to zero
	int NN = __N + __R;
	setPaddingToZero(__Gx, NN);
	setPaddingToZero(__Gy, NN);
	setPaddingToZero(__G, NN);

	// Line segment finder
	int retLS = findLineSegments(__G, __Gx, __Gy, __A, __M, lSegs, errors);
	return retLS;
	

	return RET_OK;
}
int LSWMS::computeGradientMaps(const cv::Mat &img, cv::Mat &G, cv::Mat &Gx, cv::Mat &Gy)
{
	// **********************************************
	// SOBEL mode
	//
	// This function obtains the gradient image (G, Gx, Gy),
	// and fills the angular map A.
	//
	// Args:
	// 	-> img - Grayscale input image
	// 	<- G - Gradient magnitude image
	// 	<- Gx - Gradient x-magnitude image
	// 	<- Gy - Gradient y-magnitude image
	// Ret:
	// 	RET_OK - no errors found
	// 	RET_ERROR - errors found
	// **********************************************
	if(__verbose) { printf("Compute gradient maps..."); fflush(stdout); }
	
	// Sobel operator
	int ddepth = CV_16S;
	cv::Mat absGx, absGy;

	cv::Sobel(img, Gx, ddepth, 1, 0);
	convertScaleAbs(Gx, absGx, (double)1/8);

	cv::Sobel(img, Gy, ddepth, 0, 1);
	convertScaleAbs(Gy, absGy, (double)1/8);

	//cv::addWeighted(absGx, 0.5, absGy, 0.5, 0, G, CV_8U);
	cv::add(absGx, absGy, G);

	// Obtain the threshold
	cv::Scalar meanG = cv::mean(G);
	__meanG = (int)meanG.val[0];
	if(__verbose) { printf(" computed: __meanG = %d\n", __meanG); }

	// Move from 2nd to 4th and from 3rd to 1st
	// From 2nd to 4th, 	
	//if( gx < 0 && gy > 0 ) {gx = -gx; gy = -gy;} // from 2 to 4
	//if( gx < 0 && gy < 0 ) {gx = -gx; gy = -gy;} // from 3 to 1
	//if( gx < 0 ) {gx = -gx; gy = -gy;}
	int movedCounter = 0;
	for(int j=0; j<__imPadSize.height; ++j)
	{
		short *ptRowGx = Gx.ptr<short>(j);
		short *ptRowGy = Gy.ptr<short>(j);
		for(int i=0; i<__imPadSize.width; ++i)
		{
			if(ptRowGx[i] < 0)
			{
				ptRowGy[i] = -ptRowGy[i];
				ptRowGx[i] = -ptRowGx[i];
				movedCounter++;
			}
		}
	}
	if(__verbose) { printf("Moved %d/%d (%.2f%%) elements to 1st4th quadrant\n", movedCounter, __imPadSize.height*__imPadSize.width, ((double)100*movedCounter)/((double)__imPadSize.height*__imPadSize.width)); }

	if(__meanG > 0 && __meanG < 256)
		return RET_OK;
	else
		return RET_ERROR;	
}
int LSWMS::findLineSegments(const cv::Mat &G, const cv::Mat &Gx, const cv::Mat &Gy, cv::Mat &A, cv::Mat &M, std::vector<LSEG> &lSegs, std::vector<double> &errors)
{
	// **********************************************
	// This function finds line segments using the 
	// probability map P, the gradient components 
	// Gx, and Gy and the angle map A.
	//
	// Args:
	// 	-> G - Gradient magnitude map
	// 	-> Gx - Gradient x-magnitude image
	// 	-> Gy - Gradient y-magnitude image
	//	<- M - Mask image of visited pixels
	//	<- lSegs - vector of detected line segments
	//  <- errors - vector of angular errors
	// Ret:
	// 	RET_OK - no errors found
	// 	RET_ERROR - errors found
	// **********************************************
	// Loop over the image
	int x0, y0;
	int kIterator = 0;

	int imgSize = __img.cols*__img.rows;

	while(true)
	{				
		if (kIterator == imgSize)
		{
			// This is the end
			break;
		}
		if(__numMaxLSegs == 0)
		{
			x0 = kIterator%__img.cols;
			y0 = kIterator/__img.cols;
		}
		else
		{
			x0 = __sampleIterator[kIterator]%__img.cols;
			y0 = __sampleIterator[kIterator]/__img.cols;
		}
		kIterator++;
		
		// Add padding
		x0 = x0 + __N;
		y0 = y0 + __N;
		 
		// Check mask and value
		if(__M.at<uchar>(y0,x0)==0 && G.at<uchar>(y0,x0) > __meanG)
		{
			// The sample is (x0, y0)
			cv::Point ptOrig(x0, y0);
			float gX = (float)Gx.at<short>(y0,x0);
			float gY = (float)Gy.at<short>(y0,x0);
			DIR_POINT dpOrig(ptOrig, gX, gY);		// Since it is computed from Gx, Gy, it is in 1º4º

			// Line segment generation	
			float error = 0;
			if(__verbose) { printf("-------------------------------\n"); }
			if(__verbose) { printf("Try dpOrig=(%d,%d,%.2f,%.2f)...\n", dpOrig.pt.x, dpOrig.pt.y, dpOrig.vx, dpOrig.vy); }
			int retLS = lineSegmentGeneration(dpOrig, __lSeg, error);

			if( (retLS == RET_OK) && error < MAX_ERROR )
			{
				if(__verbose) { printf("lSeg generated=(%d,%d)->(%d,%d)...\n", __lSeg[0].x, __lSeg[0].y, __lSeg[1].x, __lSeg[1].y); }
				if(__verbose) { printf("-------------------------------\n"); }
				
				lSegs.push_back(__lSeg);
				errors.push_back((double)error);
				
				if(__numMaxLSegs != 0 && lSegs.size() >= (unsigned int)__numMaxLSegs)
					break;
			}
			else
			{
				// Mark as visited
				cv::Rect w(x0-__R, y0 -__R, __N, __N);
				cv::Mat roi = __M(w);
				roi.setTo(255);					
			}			
		}
	}	

	return RET_OK;
}
int LSWMS::lineSegmentGeneration(const DIR_POINT &dpOrig, LSEG &lSeg, float &error)
{
	// **********************************************
	// Starts at dpOrig and generates lSeg
	// 
	// Args:
	// 	-> dpOrig - starting DIR_POINT 
	//	<- lSeg - detected line segment
	// Ret:
	// 	RET_OK - lSeg created
	// 	RET_ERROR - lSeg not created
	// **********************************************
	
	// Check input data
	if(dpOrig.pt.x < 0 || dpOrig.pt.x >= __G.cols || dpOrig.pt.y<0 || dpOrig.pt.y >= __G.rows)
		return RET_ERROR;

	// Find best candidate with Mean-Shift
	// -----------------------------------------------------
	DIR_POINT dpCentr = dpOrig;	
	if(__verbose)
	{
		printf("\tMean-Shift(Centr): from (%d,%d,%.2f,%.2f) to...", dpOrig.pt.x, dpOrig.pt.y, dpOrig.vx, dpOrig.vy);
		fflush(stdout);
	}
	int retMSC = weightedMeanShift(dpOrig, dpCentr, __M); /// COMO LE PASO __M, TIENE EN CUENTA SI SE HA VISITADO O NO
	if(__verbose) { printf(" (%d,%d,%.2f, %.2f)\n", dpCentr.pt.x, dpCentr.pt.y, dpCentr.vx, dpCentr.vy); }
		
	if(retMSC == RET_ERROR)	
	{
		if(__verbose) { printf("\tMean-Shift reached not a valid point\n"); }
		return RET_ERROR;	
	}
	
	// Grow in two directions from dpCentr
	// -----------------------------------------------------
	if(__verbose) { printf("\tGROW 1:"); fflush(stdout); }
	cv::Point pt1;
	float retG1 = grow(dpCentr, pt1, 1);
	float d1 = (float)((dpCentr.pt.x - pt1.x)*(dpCentr.pt.x - pt1.x) + (dpCentr.pt.y - pt1.y)*(dpCentr.pt.y - pt1.y));
	if(__verbose) { printf("\tpt1(%d,%d), dist = %.2f, error=%.4f\n", pt1.x, pt1.y, d1, retG1); }
	
	if(__verbose) { printf("\tGROW 2:"); fflush(stdout); }
	cv::Point pt2;
	float retG2 = grow(dpCentr, pt2, 2);
	float d2 = (float)((dpCentr.pt.x - pt2.x)*(dpCentr.pt.x - pt2.x) + (dpCentr.pt.y - pt2.y)*(dpCentr.pt.y - pt2.y));
	if(__verbose) { printf("\tpt2(%d,%d), dist = %.2f, error=%.4f\n", pt2.x, pt2.y, d2, retG2); }

	if(retG1 == -1 && retG2 == -1)
		return RET_ERROR;

	// Select the most distant extremum
	if(d1<d2)
	{
		pt1 = pt2;
		error = retG2;
		if(__verbose) { printf("Longest dir is 2\n"); }
	}
	else
	{
		error = retG1;
		if(__verbose) { printf("Longest dir is 1\n"); }
	}

	// Grow to the non-selected direction, with the new orientation
	float dirX = (float)(dpCentr.pt.x - pt1.x);
	float dirY = (float)(dpCentr.pt.y - pt1.y);
	float norm = sqrt(dirX*dirX + dirY*dirY);
	
	if(norm>0)
	{
		dirX = dirX/norm;
		dirY = dirY/norm;
		
		DIR_POINT dpAux(dpCentr.pt, -(-dirY), -dirX);	// DIR_POINT must be filled ALWAYS with gradient vectors
		float retG = grow(dpAux, pt2, 1);
		error = retG;
	}
	else
	{
		pt2 = dpCentr.pt;	
	}
	
	// Check
	dirX = (float)(pt1.x -pt2.x);
	dirY = (float)(pt1.y -pt2.y);
	if( sqrt(dirX*dirX + dirY*dirY) < __N)
	{
		if(__verbose) { printf("Line segment not generated: Too short.\n"); }
		return RET_ERROR;
	}
	
	// Output line segment
	if(__verbose) { printf("LSeg = (%d,%d)-(%d,%d)\n", pt2.x, pt2.y, pt1.x, pt1.y); }
	lSeg.clear();
	lSeg.push_back(cv::Point(pt2.x - 2*__R, pt2.y - 2*__R));
	lSeg.push_back(cv::Point(pt1.x - 2*__R, pt1.y - 2*__R));
	
	// Update visited positions matrix
	updateMask(pt1,pt2);
	return RET_OK;
}
void LSWMS::updateMask(cv::Point pt1, cv::Point pt2)
{
	// Bresenham from one extremum to the other
	int x1 = pt1.x, x2 = pt2.x, y1 = pt1.y, y2 = pt2.y;
	int dx = ABS(x2-x1);
	int dy = ABS(y2-y1);
	int sx, sy, err, e2;

	if(x1 < x2) sx = 1; else sx = -1;
	if(y1 < y2) sy = 1; else sy = -1;
	err = dx-dy;
	while(true)
	{
		// Current value is (x1,y1)
		// -------------------------------
		// Do...
		// Set window to "visited=255"
		for(int j=y1-__R; j<=y1+__R; ++j)
		{
			unsigned char* ptRowM = __M.ptr<uchar>(j);
			for(int i=x1-__R; i<=x1+__R; ++i)		
				ptRowM[i] = 255;				
		}
		// -------------------------------
		
		// Check end
		if (x1 == x2 && y1 == y2) break;

		// Update position for next iteration
		e2 = 2*err;
		if(e2 > -dy) { err = err - dy; x1 = x1 + sx;}
		if(e2 < dx)  { err = err + dx; y1 = y1 + sy;} 	
	}
}
int LSWMS::weightedMeanShift(const DIR_POINT &dpOrig, DIR_POINT &dpDst, const cv::Mat &M)
{
	// **********************************************
	// Refines dpOrig and creates dpDst
	// 
	// Args:
	// 	-> dpOrig - starting DIR_POINT 
	//	<- dpDst - refined DIR_POINT
	// Ret:
	// 	RET_OK - dpDst created
	// 	RET_ERROR - dpDst not found
	//
	// Called from "lineSegmentGeneration"
	// **********************************************

	// MAIN LOOP: loop until MS generates no movement (or dead-loop)
	__seeds.clear();
	DIR_POINT dpCurr = dpOrig;	// The initial dp is in 1º4º
	dpDst = dpOrig;

	while(true)
	{
		// Check point
		if(dpCurr.pt.x < 0 || dpCurr.pt.x >= __G.cols || dpCurr.pt.y<0 || dpCurr.pt.y >= __G.rows)
			return RET_ERROR;

		// Check direction
		if(dpCurr.vx==0 && dpCurr.vy == 0)	
			return RET_ERROR;

		// Convert to 1º4º (maybe not needed)
		setTo14Quads(dpCurr);

		// Check already visited
		if(!M.empty())
		{
			if(M.at<uchar>(dpCurr.pt.y, dpCurr.pt.x) == 255) 
			{
				return RET_ERROR;
			}
		}

		// Check if previously used as seed for this MS-central (this is to avoid dead-loops)
		for(unsigned int i=0; i<__seeds.size(); i++)
		{
			if(__seeds[i].x == dpCurr.pt.x && __seeds[i].y == dpCurr.pt.y)
			{
				dpDst = dpCurr;
				return RET_ERROR;
			}
		}		

		// Define bounds
		int xMin = dpCurr.pt.x - __R;
		int yMin = dpCurr.pt.y - __R;
		int xMax = dpCurr.pt.x + __R;
		int yMax = dpCurr.pt.y + __R;
		int offX = __R;
		int offY = __R;

		if( xMin < 0 || yMin < 0 || xMax >= __G.cols || yMax >= __G.rows)
			return RET_ERROR;
		
		__seeds.push_back(dpCurr.pt);
		
		// Define rois
		cv::Rect roi(xMin, yMin, xMax-xMin+1, yMax-yMin+1);
		cv::Mat gBlock = cv::Mat(__G, roi);
		cv::Mat gXBlock = cv::Mat(__Gx, roi);
		cv::Mat gYBlock = cv::Mat(__Gy, roi);
		cv::Mat aBlock = cv::Mat(__A, roi);
		cv::Mat insideBlock = cv::Mat(gBlock.size(), CV_8U); // 0: outside, 1:inside
		insideBlock.setTo(1);

		// Update angles (this is to compute angles only once)
		for(int j=0; j<aBlock.rows; ++j)
		{
			for(int i=0; i<aBlock.cols; ++i)
			{
				// This is guaranteed to be in 1º and 4º quadrant
				aBlock.at<float>(j,i) = atan2((float)gYBlock.at<short>(j,i), (float)gXBlock.at<short>(j,i));
			}
		}

		//if(__verbose) printf("dpCurr(%d,%d)(%.2f,%.2f)\n", dpCurr.pt.x, dpCurr.pt.y, dpCurr.vx, dpCurr.vy);
				
		//if(__verbose) std::cout << "gBlock" << gBlock << endl;
		//if(__verbose) std::cout << "gXBlock" << gXBlock << endl;
		//if(__verbose) std::cout << "gYBlock" << gYBlock << endl;
		//if(__verbose) std::cout << "aBlock" << aBlock << endl;

		// ----------------------------------
		// Angle analysis
		float currentAngle = atan2(dpCurr.vy, dpCurr.vx);	// output is between (-CV_PI/2, CV_PI/2)
		//if(__verbose) printf("currentAngle = %.2f\n", currentAngle);
		// ----------------------------------

		float angleShift = 0;
		int outsideCounter = 0;
		if(currentAngle - __margin < -PI_2)
		{
			// Shift angles according to currentAngle to avoid discontinuities			
			//if(__verbose) printf("shift angles since %.2f - %.2f < %.2f\n", currentAngle, __margin, -PI_2);
			angleShift = currentAngle;
			aBlock = aBlock - currentAngle;	
			currentAngle = 0;
			float minAngle = currentAngle - __margin;
			float maxAngle = currentAngle + __margin;

			for(int j=0; j<aBlock.rows; j++)
			{
				float *ptRowABlock = aBlock.ptr<float>(j);
				uchar *ptRowGBlock = gBlock.ptr<uchar>(j);
				for(int i=0; i<aBlock.cols; i++)
				{
					if(ptRowABlock[i] < -PI_2) ptRowABlock[i] += (float)CV_PI;
					if(ptRowABlock[i] > PI_2) ptRowABlock[i] -= (float)CV_PI;
					if(ptRowABlock[i] < minAngle || ptRowABlock[i] > maxAngle) 
					{
						//ptRowGBlock[i] = -1;
						insideBlock.at<uchar>(j,i) = 0;
						outsideCounter++;
					}	
				}
			}		
			// Restore
			aBlock = aBlock + angleShift;	


		}
		else if(currentAngle + __margin > PI_2)
		{
			// Shift angles according to currentAngle to avoid discontinuities
			//if(__verbose) printf("shift angles since %.2f + %.2f > %.2f\n", currentAngle, __margin, PI_2);
			angleShift = currentAngle;
			aBlock = aBlock - currentAngle;
			currentAngle = 0;

			float minAngle = currentAngle - __margin;
			float maxAngle = currentAngle + __margin;

			for(int j=0; j<aBlock.rows; j++)
			{
				float *ptRowABlock = aBlock.ptr<float>(j);
				uchar *ptRowGBlock = gBlock.ptr<uchar>(j);
				for(int i=0; i<aBlock.cols; i++)
				{
					if(ptRowABlock[i] < -PI_2) ptRowABlock[i] += (float)CV_PI;
					if(ptRowABlock[i] > PI_2) ptRowABlock[i] -= (float)CV_PI;
					if(ptRowABlock[i] < minAngle || ptRowABlock[i] > maxAngle) 
					{
						//ptRowGBlock[i] = -1;
						insideBlock.at<uchar>(j,i) = 0;
						outsideCounter++;
					}	
				}
			}
			// Restore
			aBlock = aBlock + angleShift;
		}
		else
		{
			angleShift = 0;
			float minAngle = currentAngle - __margin;
			float maxAngle = currentAngle + __margin;
			for(int j=0; j<aBlock.rows; j++)
			{
				float *ptRowABlock = aBlock.ptr<float>(j);
				uchar *ptRowGBlock = gBlock.ptr<uchar>(j);
				for(int i=0; i<aBlock.cols; i++)
				{					
					if(ptRowABlock[i] < minAngle || ptRowABlock[i] > maxAngle) 
					{
						//ptRowGBlock[i] = -1;
						insideBlock.at<uchar>(j,i) = 0;
						outsideCounter++;
					}	
				}
			}
		}

		//if(__verbose) std::cout << "insideBlock" << insideBlock << endl;
		//if(__verbose) std::cout << "aBlock(after computing insideBlock" << aBlock << endl;

		// Check number of samples inside the bandwidth
		if(outsideCounter == (2*__R+1)*(2*__R+1))
			return RET_ERROR;

		// New (Circular) Mean angle (weighted by G)
		float sumWeight = 0;
		float foffX = 0;
		float foffY = 0;
		float meanAngle = 0;
		
		for(int j=0; j<gBlock.rows; j++)
		{			
			uchar *ptRowGBlock = gBlock.ptr<uchar>(j);
			float *ptRowABlock = aBlock.ptr<float>(j);

			for(int i=0; i<gBlock.cols; i++)
			{
				//if(ptRowGBlock[i] != -1)
				if(insideBlock.at<uchar>(j,i) != 0)
				{
					// This sample is inside the Mean-Shift bandwidth
					// Weighted mean of positons
					foffX += (float)(i+1)*ptRowGBlock[i];	  // This cannot be precomputed...
					foffY += (float)(j+1)*ptRowGBlock[i];
							
					// Weighted mean of angle
					meanAngle += ptRowABlock[i]*ptRowGBlock[i];
					sumWeight += ptRowGBlock[i];		  
				}
			}
		}
		foffX /= sumWeight; foffX--;
		foffY /= sumWeight; foffY--;
		meanAngle /= sumWeight;

		//if(__verbose) printf("meanAngle = %.2f\n", meanAngle);
				
		// Check convergence (movement with respect to the center)
		if(cvRound(foffX) == offX && cvRound(foffY) == offY)
		{
			// Converged. Assign and return.
			dpDst = DIR_POINT(dpCurr.pt, cos(meanAngle), sin(meanAngle));
			setTo14Quads(dpDst);
			return RET_OK;
		}
		else
		{
			// Not converged: update dpCurr and iterate
			dpCurr.pt.x += cvRound(foffX) - offX;
			dpCurr.pt.y += cvRound(foffY) - offY;
			dpCurr.vx = cos(meanAngle);
			dpCurr.vy = sin(meanAngle);
		}

	}	

	return RET_OK;
}
float LSWMS::grow(const DIR_POINT &dpOrig, cv::Point &ptDst, int dir)
{
	// **********************************************
	// Finds end-point ptDst starting from dpOrig
	// 
	// Args:
	// 	-> dpOrig - starting DIR_POINT 
	//	<- ptDst - end-point
	//	-> dir - growing direction (1(+) or 2(-))
	// Ret:
	// 	error - error of line segment
	//
	// Called from lineSegmentGeneration
	// **********************************************	
	cv::Point ptEnd1, ptEnd2; //auxiliar
	DIR_POINT dpEnd, dpRef;	  // auxiliar

	// Init output
	ptDst = dpOrig.pt;

	// Starting gradient vector and director vector
	float gX, gY;
	if(dir == 1)
	{	
		gX = dpOrig.vx;
		gY = dpOrig.vy;		
	}
	else if(dir == 2)
	{
		gX = -dpOrig.vx;
		gY = -dpOrig.vy;
	}
	else return RET_ERROR;

	// Compute currentAngle in 1º4º
	float error1 = 0;
	float growAngle, auxAngle, minAngle, maxAngle, diffAngle;
	//if(gX < 0)	// In this case, direction must not be fliped to 1º4º, because otherwise the sense is lost for the second grow procedure...
	//{
	//	// Move to 1º4º
	//	gX = -gX;
	//	gY = -gY;
	//}
	growAngle = atan2(gY, gX);

	// Starting point and angle - Bresenham
	cv::Point pt1 = dpOrig.pt;
	cv::Point pt2(pt1.x + (int)(1000*(-gY)), pt1.y + (int)(1000*(gX)));
	cv::clipLine(__imPadSize, pt1, pt2);	
	
	// Loop	- Bresenham
	int k1=0;
	
	int x1 = pt1.x, x2 = pt2.x, y1 = pt1.y, y2 = pt2.y;
	int dx = ABS(x2-x1);
	int dy = ABS(y2-y1);
	int sx, sy, err, e2;

	if(__verbose) { printf("From (%d,%d) to (%d,%d)...", x1, y1, x2, y2); fflush(stdout); }

	if(x1 < x2) sx = 1; else sx = -1;
	if(y1 < y2) sy = 1; else sy = -1;
	err = dx-dy;
	
	int maxNumZeroPixels = 2*__R, countZeroPixels=0;
	while(true)
	{		
		// Current value is (x1,y1)	
		//if(__verbose) { printf("\n\tBresenham(%d,%d)", x1, y1); fflush(stdout); }
		// -------------------------------
		// Do...
		// Check if angle has been computed
		if(__A.at<float>(y1,x1) != NOT_A_VALID_ANGLE)
			auxAngle = __A.at<float>(y1,x1);
		else
		{
			auxAngle = atan2((float)__Gy.at<short>(y1,x1), (float)__Gx.at<short>(y1,x1));
			__A.at<float>(y1,x1) = auxAngle;
		}		

		// Check early-termination of Bresenham		
		if(__G.at<uchar>(y1,x1) == 0) 
		{
			//if(__verbose) printf("Zero-pixel num. %d\n", countZeroPixels);
			
			countZeroPixels++;
			if(countZeroPixels >= maxNumZeroPixels)
				break;		// No gradient point
			
		}
		
		// Check angular limits
		if(growAngle - __margin < -PI_2)		    // e.g. currentAngle = -80º, margin = 20º
		{
			minAngle = growAngle - __margin + (float)CV_PI; // e.g. -80 -20 +180 = 80º
			maxAngle = growAngle + __margin;	    // e.g. -80 +20      =-60º	

			if( auxAngle < 0)
			{
				if( auxAngle > maxAngle ) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
				diffAngle = ABS(growAngle - auxAngle);
			}
			else // auxAngle > 0
			{
				if( auxAngle < minAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
				diffAngle = ABS(growAngle - (auxAngle - (float)CV_PI));
			}			
		}
		else if(growAngle + __margin > PI_2)		    // e.g. currentAngle = 80º, margin = 20º
		{
			minAngle = growAngle - __margin;	    // e.g.  80 -20      = 60º
			maxAngle = growAngle + __margin - (float)CV_PI; // e.g.  80 +20 -180 = -80º

			if( auxAngle > 0 )
			{
				if( auxAngle < minAngle) break;	//if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
				diffAngle = ABS(growAngle - auxAngle);
			}
			else // auxAngle < 0
			{
				if( auxAngle > maxAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
				diffAngle = ABS(growAngle - (auxAngle + (float)CV_PI));
			}
		}
		else
		{	
			minAngle = growAngle - __margin;
			maxAngle = growAngle + __margin;
			if(auxAngle < minAngle || auxAngle > maxAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) || > maxAngle(%.2f)\n", auxAngle, minAngle, maxAngle);
			
			diffAngle = ABS(growAngle - auxAngle);
		}
		
		// If arrived here, the point is valid (inside the angular limits, and with G!=0)
		//error1 += ABS(ABS(__Gx.at<short>(y1,x1)) - ABS(gX)) +
		//	  ABS(ABS(__Gy.at<short>(y1,x1)) - ABS(gY));
		//error1 += ABS(auxAngle - growAngle); // OJO, SI HA HABIDO DISCONTINUIDAD, ESTO NO ES CORRECTO...
		error1 += diffAngle;
		ptEnd1 = cv::Point(x1,y1);
		k1++;

		// -------------------------------
		// Check end
		if (x1 == x2 && y1 == y2) break;

		// Update position for next iteration
		e2 = 2*err;
		if(e2 > -dy) { err = err - dy; x1 = x1 + sx;}
		if(e2 < dx)  { err = err + dx; y1 = y1 + sy;} 		
	}

	// "k1": how many points have been visited
	// "ptEnd": last valid point
	if( k1==0 ) // this means that even the closest point has not been accepted	
	{
		ptEnd1 = dpOrig.pt;
		error1 = (float)CV_PI;
	}	
	else error1 /= k1;
	if(__verbose) { printf(", Arrived to (%d,%d), error=%.2f", ptEnd1.x, ptEnd1.y, error1); fflush(stdout); }
	
	// Set ptDst
	ptDst = ptEnd1;

	// Apply Mean-Shift to refine the end point	
	//if(__verbose) printf("Check grow movement: From (%d,%d) to (%d,%d)\n", dpOrig.pt.x, dpOrig.pt.y, ptEnd1.x, ptEnd1.y);	
	if(__verbose) { printf(", Dist = (%d,%d)\n", ABS(ptEnd1.x - dpOrig.pt.x), ABS(ptEnd1.y - dpOrig.pt.y)); }
	if(ABS(ptEnd1.x - dpOrig.pt.x) > __R || ABS(ptEnd1.y - dpOrig.pt.y) > __R) // ONLY IF THERE HAS BEEN (SIGNIFICANT) MOTION FROM PREVIOUS MEAN-SHIFT MAXIMA
	{		
		int counter = 0;
		while(true)
		{
			if(__verbose) { printf("\tMean-Shift(Ext): from (%d,%d,%.2f,%.2f) to...", ptEnd1.x, ptEnd1.y, gX, gY); fflush(stdout); }
			counter++;

			// Mean-Shift on the initial extremum
			// -------------------------------------------------------------
			dpEnd.pt = ptEnd1; dpEnd.vx = gX; dpEnd.vy = gY;	// gX and gY have been update in the last iter
			dpRef.pt = ptEnd1; dpRef.vx = gX; dpRef.vy = gY;
			int retMSExt = weightedMeanShift(dpEnd, dpRef);

			if(__verbose) { printf("(%d,%d,%.2f,%.2f)\n", dpRef.pt.x, dpRef.pt.y, dpRef.vx, dpRef.vy); }

			if(retMSExt == RET_ERROR)
			{
				// The refinement gave and incorrect value, keep last Bresenham value
				ptDst = ptEnd1;
				return RET_OK;
			}
			

			// Check motion caused by Mean-Shift
			if(dpRef.pt.x == dpEnd.pt.x && dpRef.pt.y == dpEnd.pt.y)
			{
				ptDst = dpRef.pt;
				return RET_OK;
			}

			// Check displacement from dpOrig
			gX = (float)(dpRef.pt.y - dpOrig.pt.y);	// 	float dX = dpRef.x - dpOrig.x; and gX = dY;
			gY = (float)(dpOrig.pt.x - dpRef.pt.x);	//	float dY = dpRef.y - dpOrig.y; and gY = -dX;
			if(gX == 0 && gY == 0)
			{	
				ptDst = dpRef.pt;
				return RET_OK;
			}
			float norm = sqrt(gX*gX + gY*gY);
			gX /= norm;
			gY /= norm;	

			// New Bresenham procedure	
			if(gX < 0)
			{
				// MOve to 1º4º
				gX = -gX;
				gY = -gY;
			}
			growAngle = atan2(gY, gX);

			int k2=0;
			float error2 = 0;

			pt2.x = pt1.x + (int)(1000*(-gY)); pt2.y = pt1.y + (int)(1000*(gX));

			x1 = pt1.x; x2 = pt2.x; y1 = pt1.y; y2 = pt2.y;
			dx = ABS(x2-x1);	
			dy = ABS(y2-y1);

			if(x1 < x2) sx = 1; else sx = -1;
			if(y1 < y2) sy = 1; else sy = -1,
			err = dx-dy;

			if(__verbose) { printf("\tRefined GROW: From (%d,%d) to (%d,%d)...", x1, y1, x2, y2); fflush(stdout); }
			while(true)
			{
				// Current value is (x1,y1)	
				//if(__verbose) { printf("\n\tBresenham(%d,%d)", x1, y1); fflush(stdout); }
				
				// -------------------------------
				// Do...
				// Check if angle has been computed
				if(__A.at<float>(y1,x1) != NOT_A_VALID_ANGLE)
					auxAngle = __A.at<float>(y1,x1);
				else
				{
					auxAngle = atan2((float)__Gy.at<short>(y1,x1), (float)__Gx.at<short>(y1,x1));
					__A.at<float>(y1,x1) = auxAngle;
				}

				// Check early-termination of Bresenham		
				if(__G.at<uchar>(y1,x1) == 0) 
				{
					//if(__verbose) printf("Zero-pixel num. %d\n", countZeroPixels);
			
					countZeroPixels++;
					if(countZeroPixels >= maxNumZeroPixels)
						break;		// No gradient point			
				}
		
				// Check angular limits
				if(growAngle - __margin < -PI_2)		    // e.g. currentAngle = -80º, margin = 20º
				{
					minAngle = growAngle - __margin + (float)CV_PI; // e.g. -80 -20 +180 = 80º
					maxAngle = growAngle + __margin;	    // e.g. -80 +20      =-60º	

					if( auxAngle < 0)
					{
						if( auxAngle > maxAngle ) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
						diffAngle = ABS(growAngle - auxAngle);
					}
					else // auxAngle > 0
					{
						if( auxAngle < minAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
						diffAngle = ABS(growAngle - (auxAngle - (float)CV_PI));
					}			
				}
				else if(growAngle + __margin > PI_2)		    // e.g. currentAngle = 80º, margin = 20º
				{
					minAngle = growAngle - __margin;	    // e.g.  80 -20      = 60º
					maxAngle = growAngle + __margin - (float)CV_PI; // e.g.  80 +20 -180 = -80º

					if( auxAngle > 0 )
					{
						if( auxAngle < minAngle) break;	//if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
						diffAngle = ABS(growAngle - auxAngle);
					}
					else // auxAngle < 0
					{
						if( auxAngle > maxAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
						diffAngle = ABS(growAngle - (auxAngle + (float)CV_PI));
					}
				}
				else
				{	
					minAngle = growAngle - __margin;
					maxAngle = growAngle + __margin;
					if(auxAngle < minAngle || auxAngle > maxAngle) break; //if(__verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) || > maxAngle(%.2f)\n", auxAngle, minAngle, maxAngle);
			
					diffAngle = ABS(growAngle - auxAngle);
				}
				
				error2 += diffAngle;
				ptEnd2 = cv::Point(x1,y1);
				k2++;
				
				// -------------------------------

				// Check end
				if (x1 == x2 && y1 == y2) break;

				// Update position for next iteration
				e2 = 2*err;
				if(e2 > -dy) { err = err - dy; x1 = x1 + sx;}
				if(e2 < dx)  { err = err + dx; y1 = y1 + sy;} 		
			} // Bresenham while

			
			// "k2": how many points have been visited
			// "ptEnd2": last valid point
			if( k2==0 ) // this means that even the closest point has not been accepted	
			{
				ptEnd2 = dpOrig.pt;
				error2 = (float)CV_PI;
			}	
			else error2 = error2 / k2;			

			fflush(stdout);	// Don't really know why, but this is necessary to avoid dead loops...
			if(__verbose) { printf(", Arrived to (%d,%d), error=%.2f", ptEnd2.x, ptEnd2.y, error2); fflush(stdout); }
			if(__verbose) { printf(", Dist = (%d,%d)\n", ABS(ptEnd2.x - dpOrig.pt.x), ABS(ptEnd1.y - dpOrig.pt.y)); }

			// Compare obtained samples
			if(error1 <= error2)
			{
				ptDst = ptEnd1;
				return error1;
			}
			else
			{
				// Update ptEnd1 with ptEnd2 because it is better and iterate
				ptEnd1 = ptEnd2;
				k1 = k2;
				error1 = error2;
			}		
		} // Mean-Shift while
	}
	//else if(__verbose)
	//	printf("Not enough movement\n");

	//return RET_OK;
	return error1;
}

void LSWMS::setPaddingToZero(cv::Mat &img, int NN)
{
	cv::rectangle(img, cv::Point(0,0), cv::Point(img.cols-1, NN-1), cv::Scalar(0), CV_FILLED);
	cv::rectangle(img, cv::Point(0,0), cv::Point(NN-1, img.rows-1), cv::Scalar(0), CV_FILLED);
	cv::rectangle(img, cv::Point(0,img.rows-NN), cv::Point(img.cols-1, img.rows-1), cv::Scalar(0), CV_FILLED);
	cv::rectangle(img, cv::Point(img.cols-NN,0), cv::Point(img.cols-1, img.rows-1), cv::Scalar(0), CV_FILLED);
}
void LSWMS::drawLSegs(cv::Mat &img, std::vector<LSEG> &lSegs, cv::Scalar color, int thickness)
{
	for(unsigned int i=0; i<lSegs.size(); i++)	
		cv::line(img, lSegs[i][0], lSegs[i][1], color, thickness);					
}
void LSWMS::drawLSegs(cv::Mat &img, std::vector<LSEG> &lSegs, std::vector<double> &errors, int thickness)
{
	std::vector<cv::Scalar> colors;
	colors.push_back(CV_RGB(255,0,0));
	colors.push_back(CV_RGB(200,0,0));
	colors.push_back(CV_RGB(150,0,0));
	colors.push_back(CV_RGB(50,0,0));

	for(unsigned int i=0; i<lSegs.size(); i++)	
	{
		if(errors[i] < 0.087)  // 5º
			cv::line(img, lSegs[i][0], lSegs[i][1], colors[0], 3);					
		else if(errors[i] < 0.174) // 10º
			cv::line(img, lSegs[i][0], lSegs[i][1], colors[1], 2);
		else if(errors[i] < 0.26) // 15º
			cv::line(img, lSegs[i][0], lSegs[i][1], colors[2], 1);
		else 
			cv::line(img, lSegs[i][0], lSegs[i][1], colors[3], 1);
	}
}
