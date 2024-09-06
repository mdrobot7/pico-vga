#ifndef __PV_COMMON_H
#define __PV_COMMON_H

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define AVG(x, y) ((x + y) >> 2)

#define RANGE(x, lower, upper) ((x) > (upper) ? (upper) : ((x) < (lower) ? (lower) : (x)))

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define SWAP(x, y)                    \
  {                                   \
    uint32_t temp = (uint32_t) (x);   \
    x             = (typeof(x)) y;    \
    y             = (typeof(y)) temp; \
  }

#define GET_BIT(val, bit) ((val >> bit) & 1u)

#endif