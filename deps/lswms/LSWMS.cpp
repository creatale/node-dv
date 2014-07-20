/*
 * (C) Copyright 2010-2014 - Marcos Nieto
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Contributors:
 *     Marcos Nieto 
 *			 marcos dot nieto dot doncel at gmail dot com
 *			 Homepage: http://marcosnietoblog.wordpress.com
 */

#include "LSWMS.h"

#ifdef linux
	#include <stdio.h>
#endif

#include <iostream>
#include <stdlib.h>

#define ABS(a)	   (((a) < 0) ? -(a) : (a))

const double LSWMS::PI_2 = CV_PI/2;
const float LSWMS::ANGLE_MARGIN = 22.5f;
const float LSWMS::MAX_ERROR = 0.19625f; // ((22.5/2)*CV_PI/180

using namespace cv;
using namespace std;

void LSWMS::DIR_POINT::setTo14Quads()
{
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	if(vx < 0)
	{
		vx = -vx;
		vy = -vy;
	}	
}

// Public
LSWMS::LSWMS( cv::Size _imSize, int _R, int _numMaxLSegs, bool _useWMS, bool _verbose)
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

#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	m_useWMS = _useWMS;
	m_verbose = _verbose;

	// Init variables
	m_imSize = _imSize;
	m_imWidth = _imSize.width;
	m_imHeight = _imSize.height;
	
	m_R = _R;
	m_NumMaxLSegs = _numMaxLSegs;

	m_N = 2*m_R + 1;

	// Add padding if necessary
	//if( (m_imSize.width + 2*m_N) % 4 != 0)	
	//	m_N = m_N + ((m_imSize.width + 2*m_N) % 4)/2;		
	if( (m_imSize.width) % 4 != 0)	
		m_N = m_N + ((m_imSize.width) % 4)/2;		
	
	//m_imPadSize.width = m_imSize.width + 2*m_N;
	//m_imPadSize.height = m_imSize.height + 2*m_N;	

	// Init images
	m_img = cv::Mat(m_imSize, CV_8U);
	//m_imgPad = cv::Mat(m_imPadSize, CV_8U);
	//m_RoiRect = cv::Rect(m_N, m_N, m_imSize.width, m_imSize.height);
	m_RoiRect = cv::Rect(m_N, m_N, m_imSize.width - m_N, m_imSize.height - m_N);

	// Mask image
	//m_M = cv::Mat(m_imPadSize, CV_8U);
	m_M = Mat::ones(m_imSize, CV_8U);
	m_M.setTo(255);
	m_M(m_RoiRect).setTo(0);

	// Angle mask
	//m_A = cv::Mat(m_imPadSize, CV_32F);
	m_A = Mat( m_imSize, CV_32F);
	m_A.setTo( NOT_A_VALID_ANGLE );
	
	// Gradient images
	//m_G = cv::Mat(m_imPadSize, CV_8U);
	m_G = Mat::zeros(m_imSize, CV_8U);
	
	//m_Gx = cv::Mat(m_imPadSize, CV_16S);
	m_Gx = Mat::zeros(m_imSize, CV_16S);
	
	//m_Gy = cv::Mat(m_imPadSize, CV_16S);
	m_Gy = Mat(m_imSize, CV_16S);	

	// Iterator
	if(m_NumMaxLSegs != 0)
	{
		m_sampleIterator = vector<int>(m_imSize.width*m_imSize.height, 0);
		for(unsigned int k=0; k<m_sampleIterator.size(); k++)
			m_sampleIterator[k] = k;

		cv::randShuffle(m_sampleIterator);			
	}
	
	// Angular m_margin
	m_Margin = (float)(ANGLE_MARGIN*CV_PI/180);

	// Seeds
	m_seedsSize = 0;
}
int LSWMS::run(const cv::Mat& _img, std::vector<LSEG>& _lSegs, std::vector<double>& _errors)
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
			
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	// Clear line segment container
	_lSegs.clear();
	_errors.clear();
	m_M.setTo(255);
	m_M(m_RoiRect).setTo(0);

	// Input image to m_img
	if(_img.channels() == 3)
		cv::cvtColor(_img, m_img, CV_BGR2GRAY);
	else
		m_img = _img;

	// Add convolution borders
	//cv::copyMakeBorder(m_img, m_imgPad, m_N, m_N, m_N, m_N, cv::BORDER_REPLICATE); // This way we avoid line segments at the boundaries of the image

	// Init Mask matrix
	//m_M.setTo(255);
	//m_imgPadROI = m_M(m_RoiRect);
	//m_imgPadROI.setTo(0);

	// Compute Gradient map		
	// Call to the computation of the gradient and angle maps (SOBEL)
	//int retP = computeGradientMaps(m_imgPad, m_G, m_Gx, m_Gy);
	int retP = computeGradientMaps(m_img, m_G, m_Gx, m_Gy);
	if(retP == RET_ERROR)
	{
		if(m_verbose) {	printf("ERROR: Probability map could not be computed\n"); }
		return RET_ERROR;
	}	

	// Set padding to zero
	int NN = m_N + m_R;
	setPaddingToZero(m_Gx, NN);
	setPaddingToZero(m_Gy, NN);
	setPaddingToZero(m_G, NN);

	// Line segment finder
	int retLS = findLineSegments(m_G, m_Gx, m_Gy, m_A, m_M, _lSegs, _errors);

	return retLS;
}
void LSWMS::drawLSegs( cv::Mat& _img, const std::vector<LSEG>& _lSegs, cv::Scalar _color, int _thickness)
{
	for(unsigned int i=0; i<_lSegs.size(); i++)	
		cv::line(_img, _lSegs[i][0], _lSegs[i][1], _color, _thickness);					
}
void LSWMS::drawLSegs(cv::Mat& _img, const std::vector<LSEG>& _lSegs, const std::vector<double>& _errors, int _thickness)
{
	std::vector<cv::Scalar> colors;
	colors.push_back(CV_RGB(255,0,0));
	colors.push_back(CV_RGB(200,0,0));
	colors.push_back(CV_RGB(150,0,0));
	colors.push_back(CV_RGB(50,0,0));

	for(unsigned int i=0; i<_lSegs.size(); i++)	
	{
		if(_errors[i] < 0.087)  // 5º
			cv::line(_img, _lSegs[i][0], _lSegs[i][1], colors[0], 3);					
		else if(_errors[i] < 0.174) // 10º
			cv::line(_img, _lSegs[i][0], _lSegs[i][1], colors[1], 2);
		else if(_errors[i] < 0.26) // 15º
			cv::line(_img, _lSegs[i][0], _lSegs[i][1], colors[2], 1);
		else 
			cv::line(_img, _lSegs[i][0], _lSegs[i][1], colors[3], 1);
	}
}
// Private
int LSWMS::computeGradientMaps(const cv::Mat& _img, cv::Mat& _G, cv::Mat& _Gx, cv::Mat& _Gy)
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
		
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	if(m_verbose) { printf("Compute gradient maps..."); fflush(stdout); }
	
	// Sobel operator
	int ddepth = CV_16S;
	cv::Mat absGx, absGy;

	cv::Sobel(_img, _Gx, ddepth, 1, 0);
	convertScaleAbs(_Gx, absGx, (double)1/8);

	cv::Sobel(_img, _Gy, ddepth, 0, 1);
	convertScaleAbs(_Gy, absGy, (double)1/8);

	//cv::addWeighted(absGx, 0.5, absGy, 0.5, 0, G, CV_8U);
	cv::add(absGx, absGy, _G);	

	// Obtain the threshold
	cv::Scalar meanG = cv::mean(_G);
	if( !m_useWMS )
		m_MeanG = 3*(int)meanG.val[0];
	else
		m_MeanG = (int)meanG.val[0];
	if(m_verbose) { printf(" computed: m_MeanG = %d\n", m_MeanG); }
	
	// Move from 2nd to 4th and from 3rd to 1st
	// From 2nd to 4th, 	
	//if( gx < 0 && gy > 0 ) {gx = -gx; gy = -gy;} // from 2 to 4
	//if( gx < 0 && gy < 0 ) {gx = -gx; gy = -gy;} // from 3 to 1
	//if( gx < 0 ) {gx = -gx; gy = -gy;}
	int movedCounter = 0;
	for(int j=0; j<m_imPadSize.height; ++j)
	{
		short *ptRowGx = _Gx.ptr<short>(j);
		short *ptRowGy = _Gy.ptr<short>(j);
		for(int i=0; i<m_imPadSize.width; ++i)
		{
			if(ptRowGx[i] < 0)
			{
				ptRowGy[i] = -ptRowGy[i];
				ptRowGx[i] = -ptRowGx[i];
				movedCounter++;
			}
		}
	}
	if(m_verbose) { printf("Moved %d/%d (%.2f%%) elements to 1st4th quadrant\n", movedCounter, m_imPadSize.height*m_imPadSize.width, ((double)100*movedCounter)/((double)m_imPadSize.height*m_imPadSize.width)); }

	if(m_MeanG > 0 && m_MeanG < 256)
		return RET_OK;
	else
		return RET_ERROR;	
}
int LSWMS::findLineSegments(const cv::Mat& _G, const cv::Mat& _Gx, const cv::Mat& _Gy, cv::Mat &A, cv::Mat& _M, std::vector<LSEG>& _lSegs, std::vector<double>& _errors)
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

#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	// Loop over the image
	int x0, y0;
	int kIterator = 0;

	int imgSize = m_img.cols*m_img.rows;

	while(true)
	{				
		if (kIterator == imgSize)
		{
			// This is the end
			break;
		}
		if(m_NumMaxLSegs == 0)
		{
			x0 = kIterator%m_img.cols;
			y0 = kIterator/m_img.cols;
		}
		else
		{
			x0 = m_sampleIterator[kIterator]%m_img.cols;
			y0 = m_sampleIterator[kIterator]/m_img.cols;
		}
		kIterator++;
		
		// Add padding
		//x0 = x0 + m_N;
		//y0 = y0 + m_N;
		 
		// Check mask and value
		if(m_M.at<uchar>(y0,x0)==0 && _G.at<uchar>(y0,x0) > m_MeanG)
		{
			// The sample is (x0, y0)
			cv::Point ptOrig(x0, y0);
			float gX = (float)_Gx.at<short>(y0,x0);
			float gY = (float)_Gy.at<short>(y0,x0);
			DIR_POINT dpOrig(ptOrig, gX, gY);		// Since it is computed from Gx, Gy, it is in 1º4º

			// Line segment generation	
			float error = 0;
			if(m_verbose) { printf("-------------------------------\n"); }
			if(m_verbose) { printf("Try dpOrig=(%d,%d,%.2f,%.2f)...\n", dpOrig.pt.x, dpOrig.pt.y, dpOrig.vx, dpOrig.vy); }
			int retLS = lineSegmentGeneration(dpOrig, m_lSeg, error);

			if( (retLS == RET_OK) && error < MAX_ERROR )
			{
				if(m_verbose) { printf("lSeg generated=(%d,%d)->(%d,%d)...\n", m_lSeg[0].x, m_lSeg[0].y, m_lSeg[1].x, m_lSeg[1].y); }
				if(m_verbose) { printf("-------------------------------\n"); }
				
				_lSegs.push_back(m_lSeg);
				_errors.push_back((double)error);
				
				if(m_NumMaxLSegs != 0 && _lSegs.size() >= (unsigned int)m_NumMaxLSegs)
					break;
			}
			else
			{
				// Mark as visited
				//Rect w(x0-m_R, y0 -m_R, m_N, m_N);
				//Mat roi = m_M(w);
				//roi.setTo(255);					
				m_M(Rect(x0-m_R, y0 -m_R, m_N, m_N)).setTo(255);
			}			
		}
	}	

	return RET_OK;
}
int LSWMS::lineSegmentGeneration(const DIR_POINT& _dpOrig, LSEG& _lSeg, float& _error)
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

#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	// Check input data
	if(_dpOrig.pt.x < 0 || _dpOrig.pt.x >= m_G.cols || _dpOrig.pt.y<0 || _dpOrig.pt.y >= m_G.rows)
		return RET_ERROR;

	// Find best candidate with Mean-Shift
	// -----------------------------------------------------
	DIR_POINT dpCentr = _dpOrig;	
	if( m_useWMS )
	{
		if(m_verbose)
		{
			printf("\tMean-Shift(Centr): from (%d,%d,%.2f,%.2f) to...", _dpOrig.pt.x, _dpOrig.pt.y, _dpOrig.vx, _dpOrig.vy);
			fflush(stdout);
		}
		int retMSC = weightedMeanShift(_dpOrig, dpCentr, m_M); /// Since it is receiveing m_M, it knows if it has been visited or no
		if(m_verbose) { printf(" (%d,%d,%.2f, %.2f)\n", dpCentr.pt.x, dpCentr.pt.y, dpCentr.vx, dpCentr.vy); }
		
		if(retMSC == RET_ERROR)	
		{
			if(m_verbose) { printf("\tMean-Shift reached not a valid point\n"); }
			return RET_ERROR;	
		}
	}
	
	// Grow in two directions from dpCentr
	// -----------------------------------------------------
	if(m_verbose) { printf("\tGROW 1:"); fflush(stdout); }
	Point pt1;
	float retG1 = grow(dpCentr, pt1, 1);
	float d1 = (float)((dpCentr.pt.x - pt1.x)*(dpCentr.pt.x - pt1.x) + (dpCentr.pt.y - pt1.y)*(dpCentr.pt.y - pt1.y));
	if(m_verbose) { printf("\tpt1(%d,%d), dist = %.2f, error=%.4f\n", pt1.x, pt1.y, d1, retG1); }
	
	if(m_verbose) { printf("\tGROW 2:"); fflush(stdout); }
	Point pt2;
	float retG2 = grow(dpCentr, pt2, 2);
	float d2 = (float)((dpCentr.pt.x - pt2.x)*(dpCentr.pt.x - pt2.x) + (dpCentr.pt.y - pt2.y)*(dpCentr.pt.y - pt2.y));
	if(m_verbose) { printf("\tpt2(%d,%d), dist = %.2f, error=%.4f\n", pt2.x, pt2.y, d2, retG2); }

	if(retG1 == -1 && retG2 == -1)
		return RET_ERROR;

	// Select the most distant extremum
	if(d1<d2)
	{
		pt1 = pt2;
		_error = retG2;
		if(m_verbose) { printf("Longest dir is 2\n"); }
	}
	else
	{
		_error = retG1;
		if(m_verbose) { printf("Longest dir is 1\n"); }
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
		_error = retG;
	}
	else
	{
		pt2 = dpCentr.pt;	
	}
	
	// Check
	dirX = (float)(pt1.x -pt2.x);
	dirY = (float)(pt1.y -pt2.y);
	if( sqrt(dirX*dirX + dirY*dirY) < m_N)
	{
		if(m_verbose) { printf("Line segment not generated: Too short.\n"); }
		return RET_ERROR;
	}
	
	// Output line segment
	if(m_verbose) { printf("LSeg = (%d,%d)-(%d,%d)\n", pt2.x, pt2.y, pt1.x, pt1.y); }
	_lSeg.clear();
	//_lSeg.push_back(cv::Point(pt2.x - 2*m_R, pt2.y - 2*m_R));
	//_lSeg.push_back(cv::Point(pt1.x - 2*m_R, pt1.y - 2*m_R));
	_lSeg.push_back(Point(pt2.x, pt2.y));
	_lSeg.push_back(Point(pt1.x, pt1.y));
	
	// Update visited positions matrix
	updateMask(pt1,pt2);
	return RET_OK;
}
void LSWMS::updateMask(cv::Point _pt1, cv::Point _pt2)
{	
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	// Bresenham from one extremum to the other
	int x1 = _pt1.x, x2 = _pt2.x, y1 = _pt1.y, y2 = _pt2.y;
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
		for(int j=y1-m_R; j<=y1+m_R; ++j)
		{
			unsigned char* ptRowM = m_M.ptr<uchar>(j);
			for(int i=x1-m_R; i<=x1+m_R; ++i)		
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
int LSWMS::weightedMeanShift(const DIR_POINT& _dpOrig, DIR_POINT& _dpDst, const cv::Mat& _M)
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

#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	// MAIN LOOP: loop until MS generates no movement (or dead-loop)
	m_seeds.clear();
	m_seedsSize = 0;
	DIR_POINT dpCurr = _dpOrig;	// The initial dp is in 1º4º
	_dpDst = _dpOrig;

	while(true)
	{
		// Check point
		if(dpCurr.pt.x < 0 || dpCurr.pt.x >= m_G.cols || dpCurr.pt.y<0 || dpCurr.pt.y >= m_G.rows)
			return RET_ERROR;

		// Check direction
		if(dpCurr.vx==0 && dpCurr.vy == 0)	
			return RET_ERROR;

		// Convert to 1º4º (maybe not needed)
		//setTo14Quads(dpCurr);
		dpCurr.setTo14Quads();

		// Check already visited
		if(!_M.empty())
		{
			if(_M.at<uchar>(dpCurr.pt.y, dpCurr.pt.x) == 255) 
			{
				return RET_ERROR;
			}
		}

		// Check if previously used as seed for this MS-central (this is to avoid dead-loops)
		for(unsigned int i=0; i<m_seedsSize; i++)
		{
			if(m_seeds[i].x == dpCurr.pt.x && m_seeds[i].y == dpCurr.pt.y)
			{
				_dpDst = dpCurr;
				return RET_ERROR;
			}
		}		

		// Define bounds
		int xMin = dpCurr.pt.x - m_R;
		int yMin = dpCurr.pt.y - m_R;
		int xMax = dpCurr.pt.x + m_R;
		int yMax = dpCurr.pt.y + m_R;
		int offX = m_R;
		int offY = m_R;

		if( xMin < 0 || yMin < 0 || xMax >= m_G.cols || yMax >= m_G.rows)
			return RET_ERROR;
		
		m_seeds.push_back(dpCurr.pt);
		m_seedsSize++;
		
		// Define rois
		Rect roi(xMin, yMin, xMax-xMin+1, yMax-yMin+1);
		Mat gBlock = Mat(m_G, roi);
		Mat gXBlock = Mat(m_Gx, roi);
		Mat gYBlock = Mat(m_Gy, roi);
		Mat aBlock = Mat(m_A, roi);
		Mat insideBlock = Mat(gBlock.size(), CV_8U); // 0: outside, 1:inside
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

		//if(m_verbose) printf("dpCurr(%d,%d)(%.2f,%.2f)\n", dpCurr.pt.x, dpCurr.pt.y, dpCurr.vx, dpCurr.vy);
				
		//if(m_verbose) std::cout << "gBlock" << gBlock << endl;
		//if(m_verbose) std::cout << "gXBlock" << gXBlock << endl;
		//if(m_verbose) std::cout << "gYBlock" << gYBlock << endl;
		//if(m_verbose) std::cout << "aBlock" << aBlock << endl;

		// ----------------------------------
		// Angle analysis
		float currentAngle = atan2(dpCurr.vy, dpCurr.vx);	// output is between (-CV_PI/2, CV_PI/2)
		//if(m_verbose) printf("currentAngle = %.2f\n", currentAngle);
		// ----------------------------------

		float angleShift = 0;
		int outsideCounter = 0;
		if(currentAngle - m_Margin < -PI_2)
		{
			// Shift angles according to currentAngle to avoid discontinuities			
			//if(m_verbose) printf("shift angles since %.2f - %.2f < %.2f\n", currentAngle, m_Margin, -PI_2);
			angleShift = currentAngle;
			aBlock = aBlock - currentAngle;	
			currentAngle = 0;
			float minAngle = currentAngle - m_Margin;
			float maxAngle = currentAngle + m_Margin;

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
		else if(currentAngle + m_Margin > PI_2)
		{
			// Shift angles according to currentAngle to avoid discontinuities
			//if(m_verbose) printf("shift angles since %.2f + %.2f > %.2f\n", currentAngle, m_Margin, PI_2);
			angleShift = currentAngle;
			aBlock = aBlock - currentAngle;
			currentAngle = 0;

			float minAngle = currentAngle - m_Margin;
			float maxAngle = currentAngle + m_Margin;

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
			float minAngle = currentAngle - m_Margin;
			float maxAngle = currentAngle + m_Margin;
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

		//if(m_verbose) std::cout << "insideBlock" << insideBlock << endl;
		//if(m_verbose) std::cout << "aBlock(after computing insideBlock" << aBlock << endl;

		// Check number of samples inside the bandwidth
		if(outsideCounter == (2*m_R+1)*(2*m_R+1))
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

		//if(m_verbose) printf("meanAngle = %.2f\n", meanAngle);
				
		// Check convergence (movement with respect to the center)
		if(cvRound(foffX) == offX && cvRound(foffY) == offY)
		{
			// Converged. Assign and return.
			_dpDst = DIR_POINT(dpCurr.pt, cos(meanAngle), sin(meanAngle));
			//setTo14Quads(_dpDst);
			_dpDst.setTo14Quads();
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
float LSWMS::grow(const DIR_POINT& _dpOrig, cv::Point& _ptDst, int _dir)
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
	
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	cv::Point ptEnd1, ptEnd2; //auxiliar
	DIR_POINT dpEnd, dpRef;	  // auxiliar

	// Init output
	_ptDst = _dpOrig.pt;

	// Starting gradient vector and director vector
	float gX, gY;
	if(_dir == 1)
	{	
		gX = _dpOrig.vx;
		gY = _dpOrig.vy;		
	}
	else if(_dir == 2)
	{
		gX = -_dpOrig.vx;
		gY = -_dpOrig.vy;
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
	cv::Point pt1 = _dpOrig.pt;
	cv::Point pt2(pt1.x + (int)(1000*(-gY)), pt1.y + (int)(1000*(gX)));
	cv::clipLine(m_imPadSize, pt1, pt2);	
	
	// Loop	- Bresenham
	int k1=0;
	
	int x1 = pt1.x, x2 = pt2.x, y1 = pt1.y, y2 = pt2.y;
	int dx = ABS(x2-x1);
	int dy = ABS(y2-y1);
	int sx, sy, err, e2;

	if(m_verbose) { printf("From (%d,%d) to (%d,%d)...", x1, y1, x2, y2); fflush(stdout); }

	if(x1 < x2) sx = 1; else sx = -1;
	if(y1 < y2) sy = 1; else sy = -1;
	err = dx-dy;
	
	int maxNumZeroPixels = 2*m_R, countZeroPixels=0;
	while(true)
	{		
		// Current value is (x1,y1)	
		//if(m_verbose) { printf("\n\tBresenham(%d,%d)", x1, y1); fflush(stdout); }
		// -------------------------------
		// Do...
		// Check if angle has been computed
		if(m_A.at<float>(y1,x1) != NOT_A_VALID_ANGLE)
			auxAngle = m_A.at<float>(y1,x1);
		else
		{
			auxAngle = atan2((float)m_Gy.at<short>(y1,x1), (float)m_Gx.at<short>(y1,x1));
			m_A.at<float>(y1,x1) = auxAngle;
		}		

		// Check early-termination of Bresenham		
		if(m_G.at<uchar>(y1,x1) == 0) 
		{
			//if(m_verbose) printf("Zero-pixel num. %d\n", countZeroPixels);
			
			countZeroPixels++;
			if(countZeroPixels >= maxNumZeroPixels)
				break;		// No gradient point
			
		}
		
		// Check angular limits
		if(growAngle - m_Margin < -PI_2)		    // e.g. currentAngle = -80º, margin = 20º
		{
			minAngle = growAngle - m_Margin + (float)CV_PI; // e.g. -80 -20 +180 = 80º
			maxAngle = growAngle + m_Margin;	    // e.g. -80 +20      =-60º	

			if( auxAngle < 0)
			{
				if( auxAngle > maxAngle ) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
				diffAngle = ABS(growAngle - auxAngle);
			}
			else // auxAngle > 0
			{
				if( auxAngle < minAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
				diffAngle = ABS(growAngle - (auxAngle - (float)CV_PI));
			}			
		}
		else if(growAngle + m_Margin > PI_2)		    // e.g. currentAngle = 80º, margin = 20º
		{
			minAngle = growAngle - m_Margin;	    // e.g.  80 -20      = 60º
			maxAngle = growAngle + m_Margin - (float)CV_PI; // e.g.  80 +20 -180 = -80º

			if( auxAngle > 0 )
			{
				if( auxAngle < minAngle) break;	//if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
				diffAngle = ABS(growAngle - auxAngle);
			}
			else // auxAngle < 0
			{
				if( auxAngle > maxAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
				diffAngle = ABS(growAngle - (auxAngle + (float)CV_PI));
			}
		}
		else
		{	
			minAngle = growAngle - m_Margin;
			maxAngle = growAngle + m_Margin;
			if(auxAngle < minAngle || auxAngle > maxAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) || > maxAngle(%.2f)\n", auxAngle, minAngle, maxAngle);
			
			diffAngle = ABS(growAngle - auxAngle);
		}
		
		// If arrived here, the point is valid (inside the angular limits, and with G!=0)
		//error1 += ABS(ABS(m_Gx.at<short>(y1,x1)) - ABS(gX)) +
		//	  ABS(ABS(m_Gy.at<short>(y1,x1)) - ABS(gY));
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
		ptEnd1 = _dpOrig.pt;
		error1 = (float)CV_PI;
	}	
	else error1 /= k1;
	if(m_verbose) { printf(", Arrived to (%d,%d), error=%.2f", ptEnd1.x, ptEnd1.y, error1); fflush(stdout); }
	
	// Set ptDst
	_ptDst = ptEnd1;

	// Apply Mean-Shift to refine the end point	
	//if(m_verbose) printf("Check grow movement: From (%d,%d) to (%d,%d)\n", dpOrig.pt.x, dpOrig.pt.y, ptEnd1.x, ptEnd1.y);	
	if(m_verbose) { printf(", Dist = (%d,%d)\n", ABS(ptEnd1.x - _dpOrig.pt.x), ABS(ptEnd1.y - _dpOrig.pt.y)); }
	if(ABS(ptEnd1.x - _dpOrig.pt.x) > m_R || ABS(ptEnd1.y - _dpOrig.pt.y) > m_R) // ONLY IF THERE HAS BEEN (SIGNIFICANT) MOTION FROM PREVIOUS MEAN-SHIFT MAXIMA
	{		
		int counter = 0;
		while(true)
		{
			if(m_verbose) { printf("\tMean-Shift(Ext): from (%d,%d,%.2f,%.2f) to...", ptEnd1.x, ptEnd1.y, gX, gY); fflush(stdout); }
			counter++;

			// Mean-Shift on the initial extremum
			// -------------------------------------------------------------
			dpEnd.pt = ptEnd1; dpEnd.vx = gX; dpEnd.vy = gY;	// gX and gY have been update in the last iter
			dpRef.pt = ptEnd1; dpRef.vx = gX; dpRef.vy = gY;

			if( m_useWMS )
			{
				int retMSExt = weightedMeanShift(dpEnd, dpRef);

				if(m_verbose) { printf("(%d,%d,%.2f,%.2f)\n", dpRef.pt.x, dpRef.pt.y, dpRef.vx, dpRef.vy); }

				if(retMSExt == RET_ERROR)
				{
					// The refinement gave and incorrect value, keep last Bresenham value
					_ptDst = ptEnd1;
					return RET_OK;
				}
			}
			else
				return RET_OK;

			// Check motion caused by Mean-Shift
			if(dpRef.pt.x == dpEnd.pt.x && dpRef.pt.y == dpEnd.pt.y)
			{
				_ptDst = dpRef.pt;
				return RET_OK;
			}

			// Check displacement from dpOrig
			gX = (float)(dpRef.pt.y - _dpOrig.pt.y);	// 	float dX = dpRef.x - dpOrig.x; and gX = dY;
			gY = (float)(_dpOrig.pt.x - dpRef.pt.x);	//	float dY = dpRef.y - dpOrig.y; and gY = -dX;
			if(gX == 0 && gY == 0)
			{	
				_ptDst = dpRef.pt;
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

			if(m_verbose) { printf("\tRefined GROW: From (%d,%d) to (%d,%d)...", x1, y1, x2, y2); fflush(stdout); }
			while(true)
			{
				// Current value is (x1,y1)	
				//if(m_verbose) { printf("\n\tBresenham(%d,%d)", x1, y1); fflush(stdout); }
				
				// -------------------------------
				// Do...
				// Check if angle has been computed
				if(m_A.at<float>(y1,x1) != NOT_A_VALID_ANGLE)
					auxAngle = m_A.at<float>(y1,x1);
				else
				{
					auxAngle = atan2((float)m_Gy.at<short>(y1,x1), (float)m_Gx.at<short>(y1,x1));
					m_A.at<float>(y1,x1) = auxAngle;
				}

				// Check early-termination of Bresenham		
				if(m_G.at<uchar>(y1,x1) == 0) 
				{
					//if(m_verbose) printf("Zero-pixel num. %d\n", countZeroPixels);
			
					countZeroPixels++;
					if(countZeroPixels >= maxNumZeroPixels)
						break;		// No gradient point			
				}
		
				// Check angular limits
				if(growAngle - m_Margin < -PI_2)		    // e.g. currentAngle = -80º, margin = 20º
				{
					minAngle = growAngle - m_Margin + (float)CV_PI; // e.g. -80 -20 +180 = 80º
					maxAngle = growAngle + m_Margin;	    // e.g. -80 +20      =-60º	

					if( auxAngle < 0)
					{
						if( auxAngle > maxAngle ) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
						diffAngle = ABS(growAngle - auxAngle);
					}
					else // auxAngle > 0
					{
						if( auxAngle < minAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
						diffAngle = ABS(growAngle - (auxAngle - (float)CV_PI));
					}			
				}
				else if(growAngle + m_Margin > PI_2)		    // e.g. currentAngle = 80º, margin = 20º
				{
					minAngle = growAngle - m_Margin;	    // e.g.  80 -20      = 60º
					maxAngle = growAngle + m_Margin - (float)CV_PI; // e.g.  80 +20 -180 = -80º

					if( auxAngle > 0 )
					{
						if( auxAngle < minAngle) break;	//if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) && auxAngle > 0\n", auxAngle, minAngle);
						diffAngle = ABS(growAngle - auxAngle);
					}
					else // auxAngle < 0
					{
						if( auxAngle > maxAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) > maxAngle(%.2f) && auxAngle < 0\n", auxAngle, maxAngle);
						diffAngle = ABS(growAngle - (auxAngle + (float)CV_PI));
					}
				}
				else
				{	
					minAngle = growAngle - m_Margin;
					maxAngle = growAngle + m_Margin;
					if(auxAngle < minAngle || auxAngle > maxAngle) break; //if(m_verbose) printf("Early-termination: auxAngle(%.2f) < minAngle(%.2f) || > maxAngle(%.2f)\n", auxAngle, minAngle, maxAngle);
			
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
				ptEnd2 = _dpOrig.pt;
				error2 = (float)CV_PI;
			}	
			else error2 = error2 / k2;			

			fflush(stdout);	// Don't really know why, but this is necessary to avoid dead loops...
			if(m_verbose) { printf(", Arrived to (%d,%d), error=%.2f", ptEnd2.x, ptEnd2.y, error2); fflush(stdout); }
			if(m_verbose) { printf(", Dist = (%d,%d)\n", ABS(ptEnd2.x - _dpOrig.pt.x), ABS(ptEnd1.y - _dpOrig.pt.y)); }

			// Compare obtained samples
			if(error1 <= error2)
			{
				_ptDst = ptEnd1;
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
	//else if(m_verbose)
	//	printf("Not enough movement\n");

	//return RET_OK;
	return error1;
}
void LSWMS::setPaddingToZero(cv::Mat& _img, int _NN)
{
#if ( SHINY_PROFILER == 1 )
	PROFILE_FUNC(); // begin profile until end of block
#endif

	rectangle(_img, Point(0,0), Point(_img.cols-1, _NN-1), Scalar(0), CV_FILLED);
	rectangle(_img, Point(0,0), Point(_NN-1, _img.rows-1), Scalar(0), CV_FILLED);
	rectangle(_img, Point(0,_img.rows-_NN), Point(_img.cols-1, _img.rows-1), Scalar(0), CV_FILLED);
	rectangle(_img, Point(_img.cols-_NN,0), Point(_img.cols-1, _img.rows-1), Scalar(0), CV_FILLED);
}
