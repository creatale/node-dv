/*
 *  Delta2Binarizer.cpp
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

/*
 * The purpose of this Binarizer is to increase the resolution of a greyscale
 * luminance source in one dimension while binarizing. Because this procedure
 * needs some time, it is not suitable for total images, nor is it for square
 * 2D codes like QR Code or DataMatrix. However, it may be used for single lines
 * in 1D codes and for special lines in "stacked" 1D codes like PDF417. For
 * this purpose, it contains a function called "createLinesMatrix". Note
 * that the LuminanceSource used in this function needs a method "getStraightLine".
 * The Delta2Binarizer is suitable particularly when the bars or spaces of the
 * code are very narrow. It takes into consideration all variations of the lines
 * that are greater or equal to a specific threshold.
 */

// Delta2Binarizer.cpp: implementation of the Delta2Binarizer class.
//
//////////////////////////////////////////////////////////////////////

#include <zxing/common/Delta2Binarizer.h>
#include <zxing/common/EdgeDetector.h> /* for "Minimum" */
#include <cmath>

namespace zxing {

const unsigned int Delta2Binarizer::UNIT = 2;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Delta2Binarizer::Delta2Binarizer(Ref<LuminanceSource> source, int threshold)
: HybridBinarizer(source), Matrix_(NULL)
{
  nWidth_ = nHeight_ = 0;
  nThreshold_ = threshold; /* 16? 32? 48? */
  nMinMax_ = 0;
  bMinFirst_ = false;
  nCntBars_ = 0;
  nMaxBars_ = 0;
}

Delta2Binarizer::~Delta2Binarizer()
{
  
}

Ref<Binarizer> Delta2Binarizer::createBinarizer(Ref<LuminanceSource> source) {
  return Ref<Binarizer> (new Delta2Binarizer(source));
}

void Delta2Binarizer::ResetMatrix()
{
  if(Matrix_) {
    Matrix_.reset(NULL);
  }
}

Ref<BitMatrix> Delta2Binarizer::getBlackMatrix()
{
  if(Matrix_)
    return Matrix_;
  
  LuminanceSource& source = *getLuminanceSource();
  nWidth_ = source.getWidth();
  nHeight_ = source.getHeight();
  
  InitArrays(nWidth_);
  
  Ref<BitMatrix> newMatrix (new BitMatrix(nWidth_ * UNIT, nHeight_));
  
  ArrayRef<unsigned char> row_ref (nWidth_);
  unsigned char* row = &row_ref[0];
  
  for(int rownum=0;rownum<nHeight_;rownum++) {
    row = source.getRow(rownum, row);
    HandleRow(rownum,row,newMatrix);
  }
  
  Matrix_ = newMatrix;
  return newMatrix;
}

/**
* Create a lines matrix from the points <xTopLeft,yTopLeft>,<xTopRight,yTopRight>,
* <xBottomLeft,yBottomLeft>,<xBottomRight,yBottomRight> with nLines lines.
**/
Ref<BitMatrix> Delta2Binarizer::createLinesMatrix(
                                                  int xTopLeft,
                                                  int yTopLeft,
                                                  int xTopRight,
                                                  int yTopRight,
                                                  int xBottomLeft,
                                                  int yBottomLeft,
                                                  int xBottomRight,
                                                  int yBottomRight,
                                                  int nLines)
{
  if(Matrix_)
    return Matrix_;
  
#if (defined _WIN32 && defined DEBUG)
  OutputDebugString(L"Delta2Binarizer: createLinesMatrix\n");
#endif
  
  LuminanceSource& source = *getLuminanceSource();
  nWidth_ = Maximum(std::abs(xTopRight-xTopLeft),std::abs(xBottomRight-xBottomLeft))+1;
  nHeight_ = nLines;
  
  InitArrays(nWidth_);
  
  Ref<BitMatrix> newMatrix (new BitMatrix(nWidth_ * UNIT, nHeight_));
  
  ArrayRef<unsigned char> row_ref (nWidth_);
  unsigned char* row = &row_ref[0];
  int nDoubleHeight = nHeight_ * 2;
  
  int rownum,row_center,rest_rows;
  float xStart,yStart,xStop,yStop;
/*
  // Compute step distances for top-down traversal.
  float deltaLeftX = (xBottomLeft - xTopLeft) / nHeight_;
  float deltaLeftY = (yBottomLeft - yTopLeft) / nHeight_;
  float deltaRightX = (xBottomRight - xTopRight) / nHeight_;
  float deltaRightY = (yBottomRight - yTopRight) / nHeight_;

  // Scan rows inside the quadrilateral.
  float startX = xTopLeft;
  float startY = yTopLeft;// + 0.33 * deltaLeftY;
  float stopX = xTopRight;
  float stopY = yTopRight;// + 0.33 * deltaRightY;
  for (int rowIndex = 0; rowIndex < nHeight_; ++rowIndex) {
    if(source.getStraightLine(row, nWidth_,
                              round(startX), round(startY),
                              round(stopX), round(stopY)) > 0) {
      HandleRow(rowIndex, row, newMatrix);
    }
//    if (rowIndex == 3) {
//          std::vector<unsigned char> asd(row, (row +nWidth_));
//      int x = 0;
//    }
    startX += deltaLeftX;
    startY += deltaLeftY;
    stopX += deltaRightX;
    stopY += deltaRightY;
  }
*/
  
  /* Calculate the Euklidean distances between the left/right top and left/right bottom points ...*/
  float fSqDiff1 = xTopLeft - xTopRight;
  fSqDiff1 *= fSqDiff1;
  float fSqDiff2 = yTopLeft - yTopRight;
  fSqDiff2 *= fSqDiff2;
  float fDiff1 = sqrt(fSqDiff1 + fSqDiff2);
  fSqDiff1 = xBottomLeft - xBottomRight;
  fSqDiff1 *= fSqDiff1;
  fSqDiff2 = yBottomLeft - yBottomRight;
  fSqDiff2 *= fSqDiff2;
  float fDiff2 = sqrt(fSqDiff1 + fSqDiff2);
  
  /* ... and the ratio between these distances: */
  float fTopBotRatio = fDiff1 == 0.0 ? 0.0 : fDiff2 / fDiff1;
  float fTopBotRatioMinus1 = fTopBotRatio - 1.0;
  float fLogDiffTopBot = fTopBotRatio == 0.0 ? 0.0 : log(fTopBotRatio);
  float fLogRowDiff,fLogRowDiff2,fLogRowPos;
  
  /* If the distances are almost equal, treat the distances between two rows as equal, too: */
  if(fabs(fLogDiffTopBot) <= 0.01 || fabs(fTopBotRatioMinus1) < 1e-4) {
    for(rownum=0,row_center=1;rownum<nHeight_;rownum++, row_center+=2) {
      rest_rows = nDoubleHeight - row_center;
      xStart=(xTopLeft * rest_rows + xBottomLeft * row_center + nHeight_) / nDoubleHeight;
      yStart=(yTopLeft * rest_rows + yBottomLeft * row_center + nHeight_) / nDoubleHeight;
      xStop=(xTopRight * rest_rows + xBottomRight * row_center + nHeight_) / nDoubleHeight;
      yStop=(yTopRight * rest_rows + yBottomRight * row_center + nHeight_) / nDoubleHeight;
#if (defined _WIN32 && defined DEBUG)
      WCHAR sz[256];
      swprintf(sz,_T("rownum = %d,start=%d,%d,stop=%d,%d\n"),rownum,xStart,yStart,xStop,yStop);
      OutputDebugString(sz);
#endif
      
      /* get the line from start to end point: */
      if(source.getStraightLine(row,nWidth_,xStart,yStart,xStop,yStop) > 0)
        HandleRow(rownum,row,newMatrix);
    }
  }
  else {
  /* If the distances are not equal, at least the ratios should be equal according to the Thales theorem.
	 * We have yet calculated the logarithm of the ratio between the lengths of the top and bottom line.
     * Now create equal differences along the logarithm and apply the exponential function on them.
    */
    float fRowRatio;
    
    fLogRowDiff = fLogDiffTopBot / nDoubleHeight;
    fLogRowDiff2 = fLogRowDiff * 2.0;
    fLogRowPos = fLogRowDiff;
    
    float xLeftDiff = (float)xBottomLeft - (float)xTopLeft;
    float xRightDiff = (float)xBottomRight - (float)xTopRight;
    float yLeftDiff = (float)yBottomLeft - (float)yTopLeft;
    float yRightDiff = (float)yBottomRight - (float)yTopRight;
    float xLeftDiff2 = fTopBotRatio * (float)xTopLeft - (float)xBottomLeft;
    float xRightDiff2 = fTopBotRatio * (float)xTopRight - (float)xBottomRight;
    float yLeftDiff2 = fTopBotRatio * (float)yTopLeft - (float)yBottomLeft;
    float yRightDiff2 = fTopBotRatio * (float)yTopRight - (float)yBottomRight;
    
    for(rownum=0;rownum<nHeight_;rownum++) {
      fRowRatio = exp(fLogRowPos);
      fLogRowPos += fLogRowDiff2;
      
      xStart = round((xLeftDiff * fRowRatio + xLeftDiff2) / fTopBotRatioMinus1);
      yStart = round((yLeftDiff * fRowRatio + yLeftDiff2) / fTopBotRatioMinus1);
      xStop = round((xRightDiff * fRowRatio + xRightDiff2) / fTopBotRatioMinus1);
      yStop = round((yRightDiff * fRowRatio + yRightDiff2) / fTopBotRatioMinus1);
      
#if (defined _WIN32 && defined DEBUG)
      WCHAR sz[256];
      swprintf(sz,_T("rownum = %d,start=%d,%d,stop=%d,%d; ratio=%f\n"),rownum,xStart,yStart,xStop,yStop,fRowRatio);
      OutputDebugString(sz);
#endif
      
      if(source.getStraightLine(row,nWidth_,xStart,yStart,xStop,yStop) > 0)
        HandleRow(rownum,row,newMatrix);
    }
    
  }
  Matrix_ = newMatrix;
  return newMatrix;
}



void Delta2Binarizer::HandleRow(int rownum,unsigned char *row, Ref<BitMatrix> newMatrix)
{
  int i,j,k;
  currentRow_ = row;
  bMinFirst_ = false;

  unsigned long avgColor = 0;
  bool shouldBeBlack = true;
  unsigned char blackThreshold;
  unsigned char whiteThreshold;

  if(CalcMinMax(currentRow_)) {
    CreateBarWidths();
    k= bMinFirst_ ? 0 : anChangePoints_[0];

    // Calculate average luminance (cut off start of line)
    j = 0;
    for (i = k / UNIT; i < nWidth_; i++) {
        avgColor += row[i];
        j++;
    }
    avgColor /= j;
    // Poor man's percentile, but seems good enough
    blackThreshold = (avgColor + 0) / 2;
    whiteThreshold = (avgColor + 255) / 2;

    if (nCntBars_ < 5)
        return; // this line looks like noise

    for(i=0;i<nCntBars_;i++) {
      int leftIdx = k;
      int width = 0;
      unsigned long color = 0;

      // The bars are supposed to be alternating white/black.
      shouldBeBlack = (i % 2) == 0;

      // Get actual luminance from image data
      for(j=0;j<ausBars_[i];j++,k++) {
        if(k < nWidth_ * UNIT) {
          width++;
          color += row[k / UNIT + 1];
        }
      }
      if (width > 0) {
        color /= width;
        // Always paint black if below blackThreshold, always leave white when above
        // whiteThreshold, use decision from deltas otherwise.
        if (color < blackThreshold || (shouldBeBlack && color < whiteThreshold)) {
          for (int i=leftIdx; i<k && i < nWidth_ * UNIT; i++)
            newMatrix->set(i, rownum);
        }
      }

    }
  }
}

void Delta2Binarizer::InitArrays(int nWidth)
{
  anDiffs_ = new Array<int>(nWidth);
  anChangePoints_ = new Array<int>(nWidth+1);
  ausMinMax_ = new Array<unsigned short>(nWidth);
  ausBars_ = new Array<unsigned short>(nWidth+2);
  nMaxBars_ = nWidth+2;
}

/* Treat leftmost pixel as white and rightmost pixel as black so assumptions don't break. */
inline int pixValue(unsigned char *pRow, int i, int width) {
  if (i <= 0)
    return 255;
   else if (i >= width)
    return 0;
   else return pRow[i];
}

bool Delta2Binarizer::CalcMinMax(unsigned char *pRow)
{
  int cnt;
  bool bRising = true;
  bool bRet = false;
  
  //* 08.02.2008 hfn: Differenzberechnung schon hier.
  if(pRow != NULL) {
    for(cnt=0;cnt<nWidth_-1;cnt++) {
      anDiffs_[cnt] = pixValue(pRow, cnt+1, nWidth_) - pixValue(pRow, cnt, nWidth_);
    }
  }
  
  nMinMax_ = 0;
  
  if(pRow != NULL
    && ausMinMax_.size() >= nWidth_) {
    for(cnt=0; cnt<nWidth_;cnt++) {
      if (bRising) {
        if(pixValue(pRow, cnt+1, nWidth_) < pixValue(pRow, cnt, nWidth_)) {
          ausMinMax_[nMinMax_++] = (unsigned short) cnt;
          bRising = false;
        }
      }
      else {
        if(pixValue(pRow, cnt+1, nWidth_) > pixValue(pRow, cnt, nWidth_)) {
          ausMinMax_[nMinMax_++] = (unsigned short) cnt;
          bRising = true;
        }
      }
    }
    bRet = true;
  }
#if (defined _WIN32 && defined DEBUG && 0)
  {
    WCHAR sz[256];
    swprintf(sz,_T("pre nMinMax_=%d\n"),nMinMax_);
    OutputDebugString(sz);
  }
#endif
  return bRet;
}


void Delta2Binarizer::CalcChangePoint(int n)
{
  int idx,idxMaxDiff;
  int sum;
  int dif,maxdif;
  int leftIdx,rightIdx,lwb2,upb2;
  
  leftIdx=ausMinMax_[n];
  rightIdx=ausMinMax_[n+1];
  
  maxdif=0;
  idxMaxDiff=-1;
  for(idx=leftIdx;idx<rightIdx;idx++) {
    dif = std::abs(anDiffs_[idx]);
    if(maxdif<dif) {
      maxdif = dif;
      idxMaxDiff = idx;
    }
  }
  /* Adjust left and right border:
   * Starting from the pixel with maximum difference, a bar is all
   * pixels whose delta has the same sign */
  if(idxMaxDiff>-1) {
    lwb2 = idxMaxDiff;
    upb2 = idxMaxDiff;
    while(lwb2 >= leftIdx && anDiffs_[lwb2] * anDiffs_[idxMaxDiff] > 0)
      lwb2--;
    while(upb2 < rightIdx && anDiffs_[upb2] * anDiffs_[idxMaxDiff] > 0)
      upb2++;
    leftIdx = lwb2 + 1;
    rightIdx = upb2;
  }
  
  sum = 0;
  int quot = 0;
  int quad;
  for(idx=leftIdx;idx<rightIdx;idx++) {
    quad = anDiffs_[idx] * anDiffs_[idx];
    sum += quad * idx;
    quot += quad;
  }
  if(quot != 0)
    sum = (sum * UNIT) / quot;
  else
    sum = idxMaxDiff * UNIT;
  
  anChangePoints_[n] = sum;
  
}

void Delta2Binarizer::CalcChangePoints()
{
  int cnt;
  
  anDiffs_[nWidth_-1] = 0;
  for(cnt=0;cnt<nMinMax_-1;cnt++)
    CalcChangePoint(cnt);
}


void Delta2Binarizer::CreateBarWidths()
{
  int cnt,cnt2,cntMax;
  
  RemoveLowDiff();
  CalcChangePoints();

  for(cnt=0;cnt<nMaxBars_;cnt++)
    ausBars_[cnt] = 0;
  cnt=cnt2=0;
  if(bMinFirst_) {
    ausBars_[cnt]=(unsigned short)anChangePoints_[0];
    cnt++;
  }
  cntMax=Minimum(nMaxBars_,nMinMax_-1);
  for(;cnt<cntMax;cnt++,cnt2++) {
    ausBars_[cnt] = (unsigned short) (anChangePoints_[cnt2+1] - anChangePoints_[cnt2]);
  }

  nCntBars_ = cntMax;
}


bool Delta2Binarizer::RemoveLowestDiff()
{
  int idx,idxMinDiff=0,minDiff=256,nCmp;
  bool bRet = false;
  
  if(currentRow_ != NULL) {
    for(idx=0;idx<nMinMax_-1;idx++) {
      nCmp = std::abs((int)currentRow_[ausMinMax_[idx+1]] - (int)currentRow_[ausMinMax_[idx]]);
      if (nCmp < minDiff) {
        minDiff = nCmp;
        idxMinDiff = idx;
        bRet = true;
      }
    }
    bRet &= (minDiff < nThreshold_);
    // If minDiff found and it is below threshold, join with next section
    if(bRet) {
      if(idxMinDiff == 0) {
        bMinFirst_ ^= true;
        for(idx=1;idx<nMinMax_;idx++)
          ausMinMax_[idx-1] = ausMinMax_[idx];
        nMinMax_--;
      }
      else if(idxMinDiff == nMinMax_ - 2) {
        nMinMax_--;
      }
      else {
        for(idx=idxMinDiff+2;idx<nMinMax_;idx++)
          ausMinMax_[idx-2] = ausMinMax_[idx];
        nMinMax_-=2;
      }
    }
  }
  //	TRACE(_T("RemoveLowestDiff: nMinMax_ = %d\n"),nMinMax_);
#if (defined _WIN32 && defined DEBUG && 0)
  {
				int cnt;
                WCHAR sz[256];
                swprintf(sz,_T("nMinMax_=%d\n"),nMinMax_);
                OutputDebugString(sz);
                OutputDebugString(_T("pMin/Max: "));
                for(cnt=0;cnt<nMinMax_;cnt++) {swprintf(sz,_T("%4d,"),ausMinMax_[cnt]);
                OutputDebugString(sz);}
                OutputDebugString(_T("\n"));
                OutputDebugString(_T("MMx Val: "));
                for(cnt=0;cnt<nMinMax_;cnt++)
                  if (ausMinMax_[cnt] < nWidth_)
                  {swprintf(sz,_T("%4d,"),pucRow_[ausMinMax_[cnt]]);
                  OutputDebugString(sz);}
                  OutputDebugString(_T("\n"));
  }
#endif
  return bRet;
}

void Delta2Binarizer::RemoveLowDiff()
{
  while(RemoveLowestDiff()) {};
}

void Delta2Binarizer::SetThreshold(int n)
{
  nThreshold_ = n;
}

int Delta2Binarizer::round(float d)
{
  return (int) (d + 0.5f);
}

} /* namespace zxing */
