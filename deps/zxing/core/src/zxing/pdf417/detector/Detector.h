#ifndef __DETECTOR_PDF_H__
#define __DETECTOR_PDF_H__

/*
 *  Detector.h
 *  zxing
 *
 *  Created by Hartmut Neubauer 2012-05-25
 *  Copyright 2010,2012 ZXing authors All rights reserved.
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

#include <zxing/common/Counted.h>
#include <zxing/common/DetectorResult.h>
#include <zxing/common/BitMatrix.h>
#include <zxing/common/PerspectiveTransform.h>
#include <zxing/common/detector/WhiteRectangleDetector.h>
#include <zxing/NotFoundException.h>
#include <vector>
#include <algorithm>

namespace zxing {
namespace pdf417 {

class Detector {
private:
  static const size_t UNIT;
  static const size_t MODULES_IN_SYMBOL;
  static const size_t BARS_IN_SYMBOL;
  static const size_t POSSIBLE_SYMBOLS;
  static const int MAX_AVG_VARIANCE;
  static const int MAX_INDIVIDUAL_VARIANCE;
  static const int START_PATTERN_[];
  static const int START_PATTERN_REVERSE_[];
  static const int STOP_PATTERN_[];
  static const int STOP_PATTERN_REVERSE_[];
  static const size_t SIZEOF_START_PATTERN_;
  static const size_t SIZEOF_START_PATTERN_REVERSE_;
  static const size_t SIZEOF_STOP_PATTERN_;
  static const size_t SIZEOF_STOP_PATTERN_REVERSE_;
  static const size_t COUNT_VERTICES_;
  static const float RATIOS_TABLE[];
  static const size_t BARCODE_START_OFFSET;

  Ref<BinaryBitmap> image_;
  
  static std::vector<Ref<ResultPoint> > findVertices(Ref<BitMatrix> matrix, size_t rowStep);
  static std::vector<Ref<ResultPoint> > findVertices180(Ref<BitMatrix> matrix, size_t rowStep);
  static void RepositionVertices(std::vector<Ref<ResultPoint> > &vertices);
  static void correctVertices(Ref<BitMatrix> matrix,
                              std::vector<Ref<ResultPoint> > &vertices,
                              bool upsideDown);
  static void findWideBarTopBottom(Ref<BitMatrix> matrix,
                                     std::vector<Ref<ResultPoint> > &vertices,
                                     int offsetVertice,
                                     int startWideBar,
                                     int lenWideBar,
                                     int lenPattern,
                                     int nIncrement);
  static void findCrossingPoint(std::vector<Ref<ResultPoint> > &vertices,
                                int idxResult,
                                int idxLineA1,int idxLineA2,
                                int idxLineB1,int idxLineB2,
                                Ref<BitMatrix> matrix);
  static float computeModuleWidth(std::vector<Ref<ResultPoint> > &vertices);
  static int computeDimension(Ref<ResultPoint> topLeft,
                              Ref<ResultPoint> topRight,
                              Ref<ResultPoint> bottomLeft,
                              Ref<ResultPoint> bottomRight,
                              float moduleWidth);
  int computeYDimension(Ref<ResultPoint> topLeft,
                        Ref<ResultPoint> topRight,
                        Ref<ResultPoint> bottomLeft,
                        Ref<ResultPoint> bottomRight,
                        float moduleWidth);
  static void codewordsToBitMatrix(std::vector<std::vector<unsigned int> > &codewords, Ref<BitMatrix> &matrix);
  static int calculateClusterNumber(unsigned int codeword);
  static Ref<BitMatrix> sampleGrid(Ref<BitMatrix> image,
                                   int dimension);
  static unsigned int computeCodeword
  (const std::vector<short> &pBarWidths,
   int cwStart,
   int cwEnd);

  static ArrayRef<int> findGuardPattern(Ref<BitMatrix> matrix,
                                        size_t column,
                                        size_t row,
                                        size_t width,
                                        bool whiteFirst,
                                        const int pattern[],
                                        size_t patternSize,
                                        ArrayRef<int> counters);
  static int patternMatchVariance(ArrayRef<int> counters, const int pattern[],
                                  int maxIndividualVariance);

  static int round(float d);

public:
  Detector(Ref<BinaryBitmap> image);
  Ref<BinaryBitmap> getImage();
  Ref<DetectorResult> detect();
  Ref<DetectorResult> detect(DecodeHints const& hints);
};

}
}

#endif // __DETECTOR_PDF_H__
