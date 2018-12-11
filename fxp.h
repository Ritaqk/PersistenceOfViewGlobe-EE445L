//fxp.h

/*
Copyright 2017 Spektre
https://stackoverflow.com/users/2521214/spektre

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef __FXP_H__
#define __FXP_H__
#include <stdint.h>

uint32_t bits(uint32_t p);              // count how many bits is p
uint32_t fx32_mul(uint32_t x, uint32_t y);
uint32_t fx32_sqrt(uint32_t x);          // unsigned fixed point sqrt
uint32_t fx32_exp2(uint32_t y);          // 2^y
uint32_t fx32_log2(uint32_t x);          // = log2(x)
#endif // __FXP_H__
