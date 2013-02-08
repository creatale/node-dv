#ifndef __DECODER_RESULT_H__
#define __DECODER_RESULT_H__

/*
 *  DecoderResult.h
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
#include <zxing/common/Array.h>
#include <string>
#include <zxing/common/Str.h>

namespace zxing {

/**
 * <p>Encapsulates the result of decoding a matrix of bits. This typically
 * applies to 2D barcode formats. For now it contains the raw bytes obtained,
 * as well as a String interpretation of those bytes, if applicable.</p>
 *
 * @author Sean Owen
 */
class DecoderResult : public Counted {
private:
  ArrayRef<unsigned char> rawBytes_;
  Ref<String> text_;
  ArrayRef< ArrayRef<unsigned char> > byteSegments_;
  Ref<String> ecLevel_;

public:
  DecoderResult(ArrayRef<unsigned char> rawBytes,
                Ref<String> text,
                ArrayRef< ArrayRef<unsigned char> > byteSegments,
                Ref<String> ecLevel);

  ArrayRef<unsigned char> getRawBytes();
  Ref<String> getText();
  ArrayRef< ArrayRef<unsigned char> > getByteSegments();
  Ref<String> getECLevel();
};

inline DecoderResult::DecoderResult(ArrayRef<unsigned char> rawBytes,
                                    Ref<String> text,
                                    ArrayRef<ArrayRef<unsigned char> > byteSegments,
                                    Ref<String> ecLevel) :
  rawBytes_(rawBytes), text_(text), byteSegments_(byteSegments), ecLevel_(ecLevel) {
}

inline ArrayRef<unsigned char> DecoderResult::getRawBytes() {
  return rawBytes_;
}

inline Ref<String> DecoderResult::getText() {
  return text_;
}

inline ArrayRef< ArrayRef<unsigned char> > DecoderResult::getByteSegments() {
return byteSegments_;
}

inline Ref<String> DecoderResult::getECLevel() {
  return ecLevel_;
}

}

#endif // __DECODER_RESULT_H__
