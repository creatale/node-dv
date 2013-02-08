#if !defined(__DELTA2BINARIZER_H__)
#define __DELTA2BINARIZER_H__

/*
 *  Delta2Binarizer.h
 *  zxing
 *
 *  Originally created by: Hartmut Neubauer (hfneubauer) on 2012-06-13
 *   Schweers Informationstechnologie GmbH
 *   Meerbusch, Germany
 *   hartmut.neubauer@schweers.de
 *
 *  Copyright 2012 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Delta2Binarizer.h: interface for the Delta2Binarizer class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>
#include <zxing/Binarizer.h>
#include <zxing/LuminanceSource.h>
#include <zxing/common/Array.h>
#include <zxing/common/BitArray.h>
#include <zxing/common/BitMatrix.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>

namespace zxing {

class Delta2Binarizer : public HybridBinarizer  
{
public:
	static const unsigned int UNIT;

	void InitArrays(int nWidth);
	Delta2Binarizer(Ref<LuminanceSource> source, int threshold = 16);
	virtual ~Delta2Binarizer();
	Ref<Binarizer> createBinarizer(Ref<LuminanceSource> source);
	virtual Ref<BitMatrix> getBlackMatrix();
	virtual Ref<BitMatrix> createLinesMatrix(
		int xTopLeft,
		int yTopLeft,
		int xTopRight,
		int yTopRight,
		int xBottomLeft,
		int yBottomLeft,
		int xBottomRight,
		int yBottomRight,
		int nLines);
	void SetThreshold(int n);
	void HandleRow(int rownum,unsigned char *row, Ref<BitMatrix> newMatrix);
    void ResetMatrix();

private:
	Ref<BitMatrix> Matrix_;
	bool bMinFirst_;
	int nThreshold_;
	int nMinMax_;
	int nHeight_;
	int nWidth_;
	int nCntBars_;
	int nMaxBars_;
	ArrayRef<int> anDiffs_;
	ArrayRef<int> anChangePoints_;
	ArrayRef<unsigned short> ausMinMax_;
	ArrayRef<unsigned short> ausBars_;
    unsigned char *currentRow_;
protected:
    void RemoveLowDiff();
	bool RemoveLowestDiff();
	void CreateBarWidths();
    void CalcChangePoints();
    void CalcChangePoint(int n);
	bool CalcMinMax(unsigned char *pRow);

	static int round(float d);
};

}

#endif // !defined(__DELTA2BINARIZER_H__)
