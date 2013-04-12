#ifndef __LINESSAMPLER_H__
#define __LINESSAMPLER_H__

/*
 *  Detector.h
 *  zxing
 *
 *  Copyright 2010 ZXing authors All rights reserved.
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
#include <zxing/common/Point.h>
#include <zxing/NotFoundException.h>
#include <algorithm>
#include <vector>
#include <map>

namespace zxing {
namespace pdf417 {

class LinesSampler {
private:
  static const size_t MODULES_IN_SYMBOL;
  static const size_t BARS_IN_SYMBOL;
  static const size_t POSSIBLE_SYMBOLS;
  static const float RATIOS_TABLE[];
  static const size_t BARCODE_START_OFFSET;

  Ref<BitMatrix> linesMatrix_;
  int symbolsPerLine_;
  int dimension_;
  
  static std::vector<Ref<ResultPoint> > findVertices(Ref<BitMatrix> matrix, size_t rowStep);
  static std::vector<Ref<ResultPoint> > findVertices180(Ref<BitMatrix> matrix, size_t rowStep);

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

   Ref<BitMatrix> sampleLines(const std::vector<Ref<ResultPoint> > &vertices, int dimensionY, int dimension);

  static void codewordsToBitMatrix(std::vector<std::vector<unsigned int> > &codewords, Ref<BitMatrix> &matrix);
  static int calculateClusterNumber(unsigned int codeword);
  static Ref<BitMatrix> sampleGrid(Ref<BitMatrix> image,
                                   int dimension);
  static void computeSymbolWidths(std::vector<float>& symbolWidths,
                                  const size_t symbolsPerLine, Ref<BitMatrix> linesMatrix);
  static void linesMatrixToCodewords(std::vector<std::vector<int> > &clusterNumbers,
                                     const size_t symbolsPerLine, const std::vector<float> &symbolWidths,
                                     Ref<BitMatrix> linesMatrix, std::vector<std::vector<unsigned int> > &codewords);
  static std::vector<std::vector<std::map<unsigned int, unsigned int> > > distributeVotes(const size_t symbolsPerLine,
                                                                                          const std::vector<std::vector<unsigned int> >& codewords,
                                                                                          const std::vector<std::vector<int> >& clusterNumbers);
  static std::vector<size_t> findMissingLines(const size_t symbolsPerLine,
                                              std::vector<std::vector<unsigned int> > &detectedCodeWords);
  static int decodeRowCount(const size_t symbolsPerLine, std::vector<std::vector<unsigned int> > &detectedCodeWords,
                            std::vector<size_t> &insertLinesAt);


  static unsigned int computeCodeword(const std::vector<short> &pBarWidths,
                                      int cwStart,
                                      int cwEnd);


  static int round(float d);
  static Point intersection(Line a, Line b);

public:
  LinesSampler(Ref<BitMatrix> linesMatrix, int dimension);
  Ref<BitMatrix> sample();
};

}
}

#endif // __LINESSAMPLER_H__
