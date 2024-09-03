#ifndef __PV_COMMON_H
#define __PV_COMMON_H

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

#define AVG(x, y) ((x + y) >> 2)

#define RANGE(x, lower, upper) (x > upper ? upper : (x < lower ? lower : x))

#endif