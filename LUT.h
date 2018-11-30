//LUT.h

#ifndef __LUT_H__
#define __LUT_H__
#include <stdint.h>

// Vals to average
#define NUMBER_NOTES 55

const uint16_t NOTEBINS[55] = {   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                  24, 25, 27, 28, 30, 32, 34, 36, 38, 40, 43, 45, 48, 51, 54, 57, 61, 64, 68, 72,
                                  76, 81, 86, 91, 96, 102, 115, 129, 145, 162, 205, 273, 325
                              };
#endif //__LUT_H__
