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
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "debug.h"

static int          do_print = 0;
static int          calldepth = 0;

static void         debug_whitespace(int calldepth);
static void         debug_print_info(void);

static void
debug_whitespace(int calldepth)
{
   int                 i;

   for (i = 0; i < 2 * calldepth; i++)
      printf("-");
}

static void
debug_print_info(void)
{
   printf("e17 dbg: ");
}

void
e_debug_enter(const char *file, const char *func)
{
   if (do_print)
     {
	calldepth++;

	printf("ENTER  ");
	debug_print_info();
	debug_whitespace(calldepth);
	printf("%s, %s()\n", file, func);
	fflush(stdout);
     }
}

void
e_debug_return(const char *file, const char *func)
{
   if (do_print)
     {
	printf("RETURN ");
	debug_print_info();
	debug_whitespace(calldepth);
	printf("%s, %s()\n", file, func);
	fflush(stdout);

	calldepth--;

	if (calldepth < 0)
	   printf("NEGATIVE!!!\n");
     }
}
