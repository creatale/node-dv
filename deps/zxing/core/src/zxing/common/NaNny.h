#if (!defined __NANNY_1308471038947109837904875_INCLUDED_)
#define __NANNY_1308471038947109837904875_INCLUDED_

/*
 *  Author: Hartmut Neubauer
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
 *
 */

#include "float.h"
#include <limits>

/* NAN = "not a number", "INFINITY" = infinity */

namespace zxing {

#define isnan _isnan

#if (defined _MSC_VER)
#if _MSC_VER>=1300
static float const NAN = std::numeric_limits<float>::quiet_NaN();
static float const INFINITY = std::numeric_limits<float>::infinity();
#else
extern float __fNaN, __fInFINITY;
#define NAN			__fNaN
#define INFINITY	__fInFINITY
#endif
#endif

}

#endif		// (!defined __NANNY_1308471038947109837904875_INCLUDED_)
