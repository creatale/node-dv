// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  NoErrorException.cpp
 *  zxing
 *
 *  Created by hfneubauer, 2012-10-10.
 *  Copyright 2008-2011 ZXing authors All rights reserved.
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

#include <zxing/common/NoErrorException.h>

namespace zxing {

  /**
   * This funny exception is thrown when the PDF417 error correction tells
   * that there is no error. The reason is that the "ArrayRef<int>" /
   * "Array<int>" do not accept zero members. Maybe this is a C++ problem only.
   */
NoErrorException::NoErrorException() {}

NoErrorException::NoErrorException(const char *msg) :
    Exception(msg) {
}

NoErrorException::~NoErrorException() throw() {
}

}

