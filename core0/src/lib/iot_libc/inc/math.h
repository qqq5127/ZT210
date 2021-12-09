/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef LIB_MATCHS_H_
#define LIB_MATCHS_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVERSE_PI (0.31830988618379067153776752674503)
#define HALF_PI    (1.5707963267948966192313216916398)
#define TWO_PI     (6.283185307179586476925286766559)
#define PI         (3.1415926535897932384626433832795)   //circular constant
#define E          (2.7182818284590452353602874713527)   //napierian base
#define INVERSE_E  (0.36787944117144232159552377016146)
//the natural logarithm of 8000(the ratio of maximum frequency to minimum frequency)
#define BASE_NUMBER (8.9871968206619729803056707284276)

double mxatan(double x);
double msatan(double x);
double atan(double x);
double atan2(double x, double y);
double asin(double x);
double acos(double x);
uint32_t log2(uint32_t value);
uint32_t log2_u64(uint64_t value);
uint32_t pow(uint8_t base, uint8_t index);
int64_t pow_64(uint8_t base, uint8_t index);
int64_t mul_64(int32_t value);

double pow_double(double x);
double cos_double(double x);
double pow_cordic(double x);
double cos_cordic(double x);
float pow_float(float x);
float cos_float(float x);
float sin_float(float x);
float pow_10(float x);
float pow_e(float x);
float sqrt(float x);
#ifdef __cplusplus
}
#endif

#endif /* _LIB_MATCHS_H_ */
