//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#ifndef __CLOBAL_H
#define __CLOBAL_H

// switch memleak detection during debug on/off, scheelt veel proc.tijd
#ifdef _DEBUG
# ifndef _MEMLEAK
#  define _MEMLEAK
# endif
#endif

// dont fuck met volgorde!
#ifdef _MEMLEAK
# ifdef _DEBUG
#  ifdef _WINDOWS
#	define _CRTDBG_MAP_ALLOC
#  endif
# endif
#endif

#define _CRT_RAND_S
#include <stdlib.h>
#ifdef _MEMLEAK
# ifdef _DEBUG
#  ifdef _WINDOWS
#	include <crtdbg.h>
#   define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#  else
#   define DEBUG_NEW new
#  endif
# else
#  define DEBUG_NEW new
# endif
#else
# define DEBUG_NEW new
#endif

#ifdef __STDC_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#else
#define memcpy_s(a, b, c, d) memcpy(a, c, d)
#define sprintf_s(a, b, c, ...) sprintf(a, c, __VA_ARGS__)
#define strcpy_s(a, b, c) strcpy(a, c)
#endif
#define UINT32_MAX_VALUE  0xFFFFFFFF

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <map>
#include <set>
#include <list>
#include <string>

using std::string;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int32_t LONG;
typedef int64_t int64;

// Dit moet -na- STL shit staan
// note-to-mvl: stl is niet cool
#ifdef _MEMLEAK
# ifdef _DEBUG
#	define new DEBUG_NEW
# endif
#endif

#endif // __CLOBAL_H
