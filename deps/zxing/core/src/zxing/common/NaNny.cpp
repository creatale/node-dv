#include <zxing/common/NaNny.h>

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

namespace zxing {

#if (defined _MSC_VER)
#if _MSC_VER < 1300

float __fNaN = 0.0, __fInFINITY = 0.0;

//* The class C_BuildNAN and the static instance __buildNAN have the only
//* purpose to determine the two elements __fNaN ("Not a number") and __fInFINITY
//* (Infinity) once at the beginning.

class C_BuildNAN
{
public:
	C_BuildNAN();
};

C_BuildNAN::C_BuildNAN()
{
	unsigned long d=0x7FC00000;
	__fNaN = *(float*)&d;
	d = 0x7F800000;
	__fInFINITY = *(float*)&d;
}

static C_BuildNAN __buildNAN;

#endif
#endif

}