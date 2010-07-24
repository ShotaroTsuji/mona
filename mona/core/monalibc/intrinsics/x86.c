/*************************************************************
 * Copyright (c) 2010 Shotaro Tsuji
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is	 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************/

#include <stdint.h>

int __bitscanforward(int32_t x)
{
  int r;
  __asm__("bsfl %1, %%eax; movl %%eax, %0;"
    :"=r"(r)
    :"r"(x)
    :"%eax");
  return r;
}

int __bitscanreverse(int32_t x)
{
  int r;
  __asm__("bsrl %1, %%eax; movl %%eax, %0;"
    :"=r"(r)
    :"r"(x)
    :"%eax");
  return r;
}

int __byteswap(int32_t x)
{
  int r;
  __asm__("movl %1, %%eax; bswap %%eax; movl %%eax, %0;"
    :"=r"(r)
    :"r"(x)
    :"%eax");
  return r;
}

int __bittest(int32_t base, int offset)
{
  int r = 0;
  __asm__("movl %1, %%eax; \
  	   btl %2, %%eax; \
	   movl $0, %%eax; \
	   rcll $1, %%eax; \
	   movl %%eax, %0;"
	   :"=r"(r)
	   :"r"(base),"r"(offset)
	   :"%eax");
  return r;
}
