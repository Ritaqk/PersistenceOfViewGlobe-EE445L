//fxp.c
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


#include <stdint.h>
#include "fxp.h"
//---------------------------------------------------------------------------
const uint32_t _fx32_bits      = 32;                           // all bits count
const uint32_t _fx32_fract_bits = 3;                           // fractional bits count
const uint32_t _fx32_integ_bits = _fx32_bits - _fx32_fract_bits; // integer bits count
//---------------------------------------------------------------------------
const uint32_t _fx32_one       = 1 << _fx32_fract_bits;        // constant=1.0 (fixed point)
const uint32_t _fx32_fract_mask = _fx32_one - 1;               // fractional bits mask
const uint32_t _fx32_integ_mask = 0xFFFFFFFF - _fx32_fract_mask; // integer bits mask
const uint32_t _fx32_MSB_mask = 0x80000000;         // max unsigned bit mask
//---------------------------------------------------------------------------
uint32_t bits(uint32_t p)             // count how many bits is p
{
    uint32_t m = 0x80000000; uint32_t b = 32;
    for (; m; m >>= 1, b--)
        if (p >= m) break;
    return b;
}
//---------------------------------------------------------------------------
uint32_t fx32_mul(uint32_t x, uint32_t y)
{
    const int _h = 1; // this is MSW,LSW order platform dependent So swap 0,1 if your platform is different
    const int _l = 0;
    union _u
    {
        uint32_t u32;
        uint16_t u16[2];
    } u;
    uint32_t al, ah, bl, bh;
    uint32_t c0, c1, c2, c3;
    // separate 2^16 base digits
    u.u32 = x; al = u.u16[_l]; ah = u.u16[_h];
    u.u32 = y; bl = u.u16[_l]; bh = u.u16[_h];
    // multiplication (al+ah<<1)*(bl+bh<<1) = al*bl + al*bh<<1 + ah*bl<<1 + ah*bh<<2
    c0 = (al * bl);
    c1 = (al * bh) + (ah * bl);
    c2 = (ah * bh);
    c3 = 0;
    // propagate 2^16 overflows (backward to avoid overflow)
    c3 += c2 >> 16; c2 &= 0x0000FFFF;
    c2 += c1 >> 16; c1 &= 0x0000FFFF;
    c1 += c0 >> 16; c0 &= 0x0000FFFF;
    // propagate 2^16 overflows (normaly to recover from secondary overflow)
    c2 += c1 >> 16; c1 &= 0x0000FFFF;
    c3 += c2 >> 16; c2 &= 0x0000FFFF;
    // (c3,c2,c1,c0) >> _fx32_fract_bits
    u.u16[_l] = c0; u.u16[_h] = c1; c0 = u.u32;
    u.u16[_l] = c2; u.u16[_h] = c3; c1 = u.u32;
    c0 = (c0 & _fx32_integ_mask) >> _fx32_fract_bits;
    c0 |= (c1 & _fx32_fract_mask) << _fx32_integ_bits;
    return c0;
}
//---------------------------------------------------------------------------
uint32_t fx32_sqrt(uint32_t x) // unsigned fixed point sqrt
{
    uint32_t m, a;
    if (!x) return 0;
    m = bits(x);                // integer bits
    if (m > _fx32_fract_bits) m -= _fx32_fract_bits; else m = 0;
    m >>= 1;                    // sqrt integer result is half of x integer bits
    m = _fx32_one << m;         // MSB of result mask
    for (a = 0; m; m >>= 1)     // test bits from MSB to 0
    {
        a |= m;                 // bit set
        if (fx32_mul(a, a) > x) // if result is too big
            a ^= m;                // bit clear
    }
    return a;
}
//---------------------------------------------------------------------------
uint32_t fx32_exp2(uint32_t y)       // 2^y
{
    // handle special cases
    if (!y) return _fx32_one;                    // 2^0 = 1
    if (y == _fx32_one) return 2;                // 2^1 = 2
    uint32_t m, a, b, _y;
    // handle the signs
    _y = y & _fx32_fract_mask;  // _y fractional part of exponent
    y = y & _fx32_integ_mask;  //  y integer part of exponent
    a = _fx32_one;              // ini result
    // powering by squaring x^y
    if (y)
    {
        for (m = _fx32_MSB_mask; (m > _fx32_one) && (m > y); m >>= 1); // find mask of highest bit of exponent
        for (; m >= _fx32_one; m >>= 1)
        {
            a = fx32_mul(a, a);
            if ((uint32_t)(y & m)) a <<= 1; // a*=2
        }
    }
    // powering by rooting x^_y
    if (_y)
    {
        for (b = 2 << _fx32_fract_bits, m = _fx32_one>>1; m; m >>= 1) // use only fractional part
        {
            b = fx32_sqrt(b);
            if ((uint32_t)(_y & m)) a = fx32_mul(a, b);
        }
    }
    return a;
}
//---------------------------------------------------------------------------
uint32_t fx32_log2(uint32_t x)    // = log2(x)
{
    uint32_t y, m;
    // binary search from highest possible integer power of 2 to avoid overflows (log2(integer bits)-1)
    for (y = 0, m = _fx32_one << (bits(_fx32_integ_bits) - 1); m; m >>= 1)
    {
        y |= m; // set bit
        if (fx32_exp2(y) > x) y ^= m; // clear bit if result too big
    }
    return y;
}
//---------------------------------------------------------------------------
