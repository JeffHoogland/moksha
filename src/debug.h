/*

Copyright (C) 2000, 2001 Christian Kreibich <cK@whoop.org>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef E_DEBUG_H
#define E_DEBUG_H

#include <unistd.h>
#include <stdio.h>

/*
#undef DEBUG
*/

#ifdef DEBUG

#define D(fmt, args...) printf(fmt, ## args);
#else
#define D(msg, args...)
#endif

#ifdef DEBUG_NEST

void e_debug_enter(const char *file, const char *func);
void e_debug_return(const char *file, const char *func);

#define D_ENTER e_debug_enter(__FILE__, __FUNCTION__)

#define D_RETURN \
{ \
  e_debug_return(__FILE__, __FUNCTION__); \
  return; \
}

#define D_RETURN_(x) \
{ \
  e_debug_return(__FILE__, __FUNCTION__); \
\
  return x; \
}
#else
#define D_ENTER
#define D_RETURN       return
#define D_RETURN_(x)   return (x)
#endif

#endif 

