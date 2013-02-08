#ifndef __DETECTOR_RESULT_H__
#define __DETECTOR_RESULT_H__

/*
 *  DetectorResult.h
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

#include <vector>
#include <zxing/common/Counted.h>
#include <zxing/common/Array.h>
#include <zxing/common/BitMatrix.h>
#include <zxing/common/PerspectiveTransform.h>
#include <zxing/ResultPoint.h>

namespace zxing {

class DetectorResult : public Counted {
private:
  Ref<BitMatrix> bits_;
  std::vector<Ref<ResultPoint> > points_;
  zxing::Ref<zxing::PerspectiveTransform> perspectiveTransform_;

public:
        DetectorResult(Ref<BitMatrix> bits, std::vector<Ref<ResultPoint> > points,
          zxing::Ref<zxing::PerspectiveTransform> perspectiveTransform);

        DetectorResult(Ref<BitMatrix> bits, std::vector<Ref<ResultPoint> > points);
  Ref<BitMatrix> getBits();
  std::vector<Ref<ResultPoint> > getPoints();
  zxing::Ref<zxing::PerspectiveTransform> getPerspectiveTransform();
};
}

#endif // __DETECTOR_RESULT_H__
