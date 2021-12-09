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
#include "types.h"
#include "math.h"

const double sq2p1 = 2.414213562373095048802e0;
const double sq2m1 = .414213562373095048802e0;
const double p4 = .161536412982230228262e2;
const double p3 = .26842548195503973794141e3;
const double p2 = .11530293515404850115428136e4;
const double p1 = .178040631643319697105464587e4;
const double p0 = .89678597403663861959987488e3;
const double q4 = .5895697050844462222791e2;
const double q3 = .536265374031215315104235e3;
const double q2 = .16667838148816337184521798e4;
const double q1 = .207933497444540981287275926e4;
const double q0 = .89678597403663861962481162e3;
const double PIO2 = 1.5707963267948966135E0;

const float math_value[8] = {1.023f, 1.445f, 2.042f, 2.884f, 4.074f, 5.754f, 8.035f, 11.35f};
const float factorial[12] = {1,          1,           0.5,         0.16666667,
                             0.04166667, 0.00833333,  0.00138889,  0.00019841,
                             0.0000248,  0.000002756, 0.000000276, 0.000000025};
const int8_t signal[6] = {1, -1, 1, -1, 1, -1};
const double tangent[28] = {1,
                            0.5,
                            0.25,
                            0.125,
                            0.0625,
                            0.03125,
                            0.015625,
                            0.0078125,
                            0.00390625,
                            0.001953125,
                            0.0009765625,
                            0.00048828125,
                            0.000244140625,
                            0.0001220703125,
                            0.00006103515625,
                            0.000030517578125,
                            0.0000152587890625,
                            0.00000762939453125,
                            0.000003814697265625,
                            0.0000019073486328125,
                            0.00000095367431640625,
                            0.000000476837158203125,
                            0.0000002384185791015625,
                            0.00000011920928955078125,
                            0.000000059604644775390625,
                            0.0000000298023223876953125,
                            0.00000001490116119384765625,
                            0.000000007450580596923828125};

static float sqrt_newton_iterative(float x)
{
    union {
        float x;
        int i;
    } u;
    u.x = x;
    u.i = 0x5f3759df - (u.i >> 1);
    return u.x * (1.5f - 0.5f * x * u.x * u.x) * x;
}

double mxatan(double x)
{
    double argsq, value;

    argsq = x * x;
    value = ((((p4 * argsq + p3) * argsq + p2) * argsq + p1) * argsq + p0);
    value = value / (((((argsq + q4) * argsq + q3) * argsq + q2) * argsq + q1) * argsq + q0);
    return value * x;
}

double msatan(double x)
{
    if (x < sq2m1) {
        return mxatan(x);
    }
    if (x > sq2p1) {
        return PIO2 - mxatan(1 / x);
    }
    return PIO2 / 2 + mxatan((x - 1) / (x + 1));
}

double atan(double x)
{
    if (x > 0) {
        return msatan(x);
    }
    return -msatan(-x);
}

double atan2(double x, double y)
{
    if (x + y == x) {
        if (x >= 0) {
            return PIO2;
        }
        return -PIO2;
    }
    x = atan(x / y);
    if (y < 0) {
        if (x <= 0) {
            return x + PI;
        }
        return x - PI;
    }
    return x;
}

double asin(double x)
{
    double tmp;
    int sign;

    sign = 0;
    if (x < 0) {
        x = -x;
        sign++;
    }
    if (x > 1) {
        assert(0);
    }
    tmp = sqrt_newton_iterative(1 - x * x);
    if (x > 0.7) {
        tmp = PIO2 - atan(tmp / x);
    } else {
        tmp = atan(x / tmp);
    }
    if (sign > 0) {
        tmp = -tmp;
    }
    return tmp;
}

double acos(double x)
{
    if (x > 1 || x < -1) {
        assert(0);
    }
    return PIO2 - asin(x);
}

uint32_t pow(uint8_t base, uint8_t index)
{
    uint32_t power = 1;
    for (int i = 0; i < index; i++) {
        power = base * power;
    }
    return power;
}

uint32_t log2(uint32_t value)
{
    uint32_t temp = value;
    uint32_t ret_log2 = 0;

    while (temp > 1) {
        temp /= 2;
        ret_log2++;
    }
    return ret_log2;
}

uint32_t log2_u64(uint64_t value)
{
    uint64_t temp = value;
    uint32_t ret_log2 = 0;

    while (temp > 1) {
        temp /= 2;
        ret_log2++;
    }
    return ret_log2;
}

int64_t pow_64(uint8_t base, uint8_t index)
{
    int64_t power = 1;
    for (int i = 0; i < index; i++) {
        power = base * power;
    }
    return power;
}

int64_t mul_64(int32_t value)
{
    return (((int64_t)value) * 100000000UL);
}

double pow_cordic(double x)
{
    uint8_t n = 28;   //iterations of cordic
    uint8_t flag = 0;
    uint8_t k = 1;
    double temp = 1.0;
    double result = 1.0;
    uint16_t y;

    x = x * BASE_NUMBER;

    y = (uint16_t)x;
    x = x - y;
    if (y > 0) {
        for (uint16_t i = 1; i <= y; i++) {
            temp *= E;
        }
    }

    const double angle[] = {0.54930614433405, 0.25541281188299, 0.12565721414045, 0.062581571477003,
                            0.03126017849066, 0.01562627175205, 0.00781265895154, 0.00390626986839,
                            0.00195312748353, 0.00097656281044, 0.00048828128880, 0.00024414062985,
                            0.00012207031310, 0.00006103515632, 0.00003051757813, 0.00001525878906,
                            0.00000762939453, 0.00000381469726, 0.00000190734863, 0.00000095367431,
                            0.00000047683715, 0.00000023841858, 0.00000011920929, 0.00000005960464,
                            0.00000002980232, 0.00000001490116, 0.00000000745058, 0.00000000372529};
    int8_t signal = 1;
    double x_cosh, y_sinh, x_temp, y_temp;
    x_temp = 1.2074970678567864;
    y_temp = 0.0;   //k=1.2051363585399921

    for (uint8_t i = 1; i < n; i++) {
        x_cosh = x_temp + signal * y_temp * tangent[i];
        y_sinh = y_temp + signal * x_temp * tangent[i];
        x = x - signal * angle[i - 1];
        //printf("%d\n", i);
        if (i == 3 * k + 1 && flag == 0) {
            k = i;
            i -= 1;
            flag = 1;
        } else {
            flag = 0;
        }

        x_temp = x_cosh;
        y_temp = y_sinh;

        if (x > 0)
            signal = +1;
        else
            signal = -1;
    }
    result = x_cosh + y_sinh;
    result *= temp;

    return result;
}

double cos_cordic(double x)
{
    uint8_t n = 15;   //iterations of cordic
    uint8_t flag = 0;

    if (x < 0) {
        x = -x;
    }
    uint16_t period = (uint16_t)(x * INVERSE_PI);
    if ((uint16_t)(period * 0.5) == period * 0.5 && period != 0) {
        x = x - period * PI;
    } else if (period != 0) {
        x = (period + 1) * PI - x;
    }

    if (x > HALF_PI) {
        x = PI - x;
        flag = 1;
    }

    const double angle[] = {0.78539816339745, 0.46364760900081, 0.24497866312686, 0.12435499454676,
                            0.06241880999596, 0.03123983343027, 0.01562372862048, 0.00781234106010,
                            0.00390623013197, 0.00195312251648, 0.00097656218956, 0.00048828121119,
                            0.00024414062015, 0.00012207031189, 0.00006103515617, 0.00003051757812,
                            0.00001525878906, 0.00000762939453, 0.00000381469727, 0.00000190734863,
                            0.00000095367432, 0.00000047683716, 0.00000023841858, 0.00000011920929,
                            0.00000005960464, 0.00000002980232, 0.00000001490116, 0.00000000745058};
    int8_t signal = 1;
    double x_cos, y_sin, x_temp, y_temp;
    x_temp = 0.60725293500888;
    y_temp = 0.0;

    for (uint8_t i = 0; i < n; i++) {
        x_cos = x_temp - signal * y_temp * tangent[i];
        y_sin = y_temp + signal * x_temp * tangent[i];
        x = x - signal * angle[i];

        x_temp = x_cos;
        y_temp = y_sin;

        if (x > 0)
            signal = +1;
        else
            signal = -1;
    }
    if (flag == 1) {
        x_cos = -x_cos;
    }

    return x_cos;
}

double pow_double(double x)
{
    uint8_t n = 10;   //Order of Taylor expansion
    double result = 1.0;
    double temp = 1.0;
    double result_temp = 1.0;
    double x_pow;
    uint8_t y;
    x = x * BASE_NUMBER;
    if (x == 0) {
        result = 1.0;
    } else {
        y = (uint8_t)x;
        x = x - y;
        if (y > 0) {
            for (uint8_t i = 1; i <= y; i++) {
                temp *= E;
            }
        }

        x_pow = x;
        //Taylor expansion of exponential functions
        for (uint8_t i = 1; i <= n; i++) {
            result_temp += x_pow * factorial[i];
            x_pow *= x;
        }
        result = result_temp * temp;
    }

    return result;
}

double cos_double(double x)
{
    uint8_t n = 5;   //Order of Taylor expansion
    double result = 1.0;
    uint8_t flag = 0;
    double x_temp, x_pow, x_pow2;
    if (x < 0) {
        x = -x;
    }
    uint16_t period = (uint16_t)(x * INVERSE_PI);
    if ((uint16_t)(period * (float)0.5) == period * (float)0.5 && period != 0) {
        x_temp = x - period * PI;
    } else if (period != 0) {
        x_temp = (period + 1) * PI - x;
    } else {
        x_temp = x;
    }

    if (x_temp > HALF_PI) {
        x_temp = PI - x_temp;
        flag = 1;
    }

    x_pow = x_temp * x_temp;
    x_pow2 = x_pow;
    //Taylor expansion of cosine function
    for (uint8_t i = 1; i <= n; i++) {
        result += signal[i] * x_pow * factorial[2 * i];
        x_pow *= x_pow2;
    }
    if (flag == 1) {
        result = -result;
    }

    return result;
}

static float pow_float_e(float x, double base)
{
    uint8_t n = 10;   //Order of Taylor expansion
    float result = 1.0;
    float temp = 1.0;
    float x_pow;
    uint8_t y;
    x = x * base;
    if (x == 0) {
        result = 1.0;
    } else {
        y = (uint8_t)x;
        x = x - y;
        if (y > 0) {
            for (uint8_t i = 1; i <= y; i++) {
                temp *= E;
            }
        }

        x_pow = x;
        //Taylor expansion of exponential functions
        for (uint8_t i = 1; i <= n; i++) {
            result += x_pow * factorial[i];
            x_pow *= x;
        }
        result *= temp;
    }

    return result;
}

float pow_e(float x)
{
    return pow_float_e(x, 1);
}

float pow_float(float x)
{
    return pow_float_e(x, BASE_NUMBER);
}

float cos_float(float x)
{
    uint8_t n = 5;   //Order of Taylor expansion
    float result = 1.0;
    uint8_t flag = 0;
    float x_pow, x_pow2;
    if (x < 0) {
        x = -x;
    }
    uint16_t period = (uint16_t)(x * INVERSE_PI);
    if ((uint16_t)(period * (float)0.5) == period * (float)0.5 && period != 0) {
        x = x - period * PI;
    } else if (period != 0) {
        x = (period + 1) * PI - x;
    }

    if (x > HALF_PI) {
        x = PI - x;
        flag = 1;
    }

    x_pow = x * x;
    x_pow2 = x_pow;
    //Taylor expansion of cosine function
    for (uint8_t i = 1; i <= n; i++) {
        result += signal[i] * x_pow * factorial[2 * i];
        x_pow *= x_pow2;
    }
    if (flag == 1) {
        result = -result;
    }

    return result;
}

float sin_float(float x)
{
    uint8_t n = 5;   //Order of Taylor expansion
    float result = 1.0;
    uint8_t flag = 0;
    float x_pow, x_pow2;
    if (x < 0) {
        x = -x;
        flag++;
    }
    uint16_t period = (uint16_t)(x * INVERSE_PI);
    if ((uint16_t)(period * (float)0.5) == period * (float)0.5 && period != 0) {
        x = x - period * PI;
    } else if (period != 0) {
        x = (period + 1) * PI - x;
        flag++;
    }

    if (x > HALF_PI) {
        x = PI - x;
    }

    x_pow = x * x;
    x_pow2 = x_pow;
    //Taylor expansion of cosine function
    for (uint8_t i = 1; i <= n; i++) {
        result += signal[i] * x_pow * factorial[2 * i + 1];
        x_pow *= x_pow2;
    }
    result = result * x;
    if (flag == 1) {
        result = -result;
    }

    return result;
}

float pow_10(float x)
{
    uint8_t n = 10;   //Order of Taylor expansion
    float result = 1.0;
    float temp = 1.0;
    float result_temp = 1.0;
    float x_pow;
    int8_t y;
    x = x * 2.302585092994045684f;
    if (x == 0) {
        result = 1.0;
    } else {
        if (x > 0) {
            y = (int8_t)x;
            x = x - y;
            if (y > 0) {
                for (uint8_t i = 1; i <= y; i++) {
                    temp *= E;
                }
            }
        } else {
            y = -(int8_t)x + 1;
            x = x + y;
            if (y > 0) {
                for (uint8_t i = 1; i <= y; i++) {
                    temp *= INVERSE_E;
                }
            }
        }

        x_pow = x;
        //Taylor expansion of exponential functions
        for (uint8_t i = 1; i <= n; i++) {
            result_temp += x_pow * factorial[i];
            x_pow *= x;
        }
        result = result_temp * temp;
    }

    return result;
}

float sqrt(float x) 
{
    float new_guess;
    float last_guess;
 
    if (x < 0) {
        return -1;
    }
 
    if(x==0) return 0;
 
    new_guess = 1;
    do {
        last_guess = new_guess;
        new_guess = (last_guess + x / last_guess) / 2;
    } while (new_guess != last_guess);
 
    return new_guess;
}
