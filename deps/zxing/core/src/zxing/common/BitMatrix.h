// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
#ifndef __BIT_MATRIX_H__
#define __BIT_MATRIX_H__

/*
 *  BitMatrix.h
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
#include <zxing/common/BitArray.h>
#include <zxing/common/Array.h>
#include <limits>

namespace zxing {

class BitMatrix : public Counted {
private:
  size_t width_;
  size_t height_;
  size_t words_;
  size_t rowSize_;
  unsigned int* bits_;

#define ZX_LOG_DIGITS(digits) \
    ((digits == 8) ? 3 : \
     ((digits == 16) ? 4 : \
      ((digits == 32) ? 5 : \
       ((digits == 64) ? 6 : \
        ((digits == 128) ? 7 : \
         (-1))))))

  static const unsigned int bitsPerWord;
  static const unsigned int logBits;
  static const unsigned int bitsMask;

public:
  BitMatrix(size_t dimension);
  BitMatrix(size_t width, size_t height);

  ~BitMatrix();

  bool get(size_t x, size_t y) const {
    size_t offset = x + width_ * y;
    return ((bits_[offset >> logBits] >> (offset & bitsMask)) & 0x01) != 0;
  }

  void set(size_t x, size_t y) {
    size_t offset = x + width_ * y;
    bits_[offset >> logBits] |= 1 << (offset & bitsMask);
  }

  void flip(size_t x, size_t y);
  void clear();
  void setRegion(size_t left, size_t top, size_t width, size_t height);
  Ref<BitArray> getRow(int y, Ref<BitArray> row);

  size_t getDimension() const;
  size_t getWidth() const;
  size_t getHeight() const;

  unsigned int* getBits() const;
  
  /* 2012-05-29 hfn added: */
  ArrayRef<int> getTopLeftOnBit();
  ArrayRef<int> getBottomRightOnBit();
  

  friend std::ostream& operator<<(std::ostream &out, const BitMatrix &bm);
  const char *description();
  const char *descriptionROW(int row);

  void writePng(const char *filename, int zoom, int zoomv = 0);

private:
  BitMatrix(const BitMatrix&);
  BitMatrix& operator =(const BitMatrix&);
};

}

#endif // __BIT_MATRIX_H__
