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

#ifndef _LSWMS_H_
#define _LSWMS_H_

enum {RET_OK, RET_ERROR};

#include "opencv2/core/core.hpp"
//#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "float.h"

#define SHINY_PROFILER FALSE
#if ( SHINY_PROFILER == 1 )
#include "Shiny.h"
#endif

class LSWMS
{
	struct DIR_POINT
	{
		cv::Point pt;
		float vx;
		float vy;

		DIR_POINT(cv::Point _pt, float _vx, float _vy)
		{
			pt = _pt;
			vx = _vx;
			vy = _vy;
		};
		DIR_POINT()
		{	
			pt = cv::Point(0,0);
			vx = 0;
			vy = 0;
		}

		void setTo14Quads();
	};	

public:
	typedef std::vector<cv::Point> LSEG;

	LSWMS(cv::Size _imSize, int _R, int _numMaxLSegs = 0, bool _useWMS = false, bool _verbose = false);
	int run(const cv::Mat& _img, std::vector<LSEG>& _lSegs, std::vector<double>& _errors);
	void drawLSegs( cv::Mat& _img, const std::vector<LSEG>& _lSegs, cv::Scalar _color = CV_RGB(255,0,0), int thickness=1);
	void drawLSegs( cv::Mat& _img, const std::vector<LSEG>& _lSegs, const std::vector<double>& _errors, int _thickness = 1);

private:
	// Functions
	void setPaddingToZero(cv::Mat& _img, int _NN);
	void updateMask(cv::Point _pt1, cv::Point _pt2);

	// SOBEL-specific functions
	int computeGradientMaps(const cv::Mat& _img, cv::Mat& _G, cv::Mat& _Gx, cv::Mat& _Gy);

	// General functions
	int findLineSegments(const cv::Mat& _G, const cv::Mat& _Gx, const cv::Mat& _Gy, cv::Mat& _A, cv::Mat& _M, std::vector<LSEG>& _lSegs, std::vector<double>& _errors);
	int lineSegmentGeneration(const DIR_POINT& _dpOrig, LSEG& _lSeg, float& _error);

	// Growing and Mean-Shift
	float grow(const DIR_POINT& _dpOrig, cv::Point& _ptDst, int _dir);
	int weightedMeanShift(const DIR_POINT& _dpOrig, DIR_POINT& _dpDst, const cv::Mat& _M = cv::Mat() );

	//Verbose
	bool m_verbose;	

	// Weighted-mean shift
	bool m_useWMS;
	
	// Sizes
	cv::Size m_imSize, m_imPadSize;
	int m_imWidth, m_imHeight;

	// Window parameters	
	int m_R, m_N;

	// Line Segments	
	LSEG m_lSeg;
	int m_NumMaxLSegs;

	// Images and maps
	cv::Mat m_img, m_imgPad;
	cv::Rect m_RoiRect;
	//cv::Mat m_imgPadROI;
	cv::Mat m_G, m_Gx, m_Gy;	// Map of probability of line segments
	cv::Mat m_M;			// Map of visited pixels
	cv::Mat m_A;			// Map of angles

	cv::Mat m_imAux;		// For debugging

	// Thresholds and variables
	int m_MeanG;
	std::vector<int> m_sampleIterator;
	float m_Margin;

	// Mean-Shift central
	std::vector<cv::Point> m_seeds;
	std::size_t m_seedsSize;

	// Constants
	static const int NOT_A_VALID_ANGLE = 5;
	static const float ANGLE_MARGIN;
	static const float MAX_ERROR;
	static const double PI_2;
};

#endif // _LSWMS_H_
