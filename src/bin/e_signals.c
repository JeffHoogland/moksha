/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 * NOTE TO FreeBSD users. Install libexecinfo from 
 * ports/devel/libexecinfo and add -lexecinfo to LDFLAGS
 * to add backtrace support.
 */
#include "e.h"

#ifdef OBJECT_PARANOIA_CHECK   

/* a tricky little devil, requires e and it's libs to be built
 * with the -rdynamic flag to GCC for any sort of decent output. 
 */
EAPI void
e_sigseg_act(int x, siginfo_t *info, void *data)
{
   void *array[255];
   size_t size;
   
   write(2, "**** SEGMENTATION FAULT ****\n", 29);
   write(2, "**** Printing Backtrace... *****\n\n", 34);
   size = backtrace(array, 255);
   backtrace_symbols_fd(array, size, 2);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SEGV'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11); 
}
#else
EAPI void
e_sigseg_act(int x, siginfo_t *info, void *data)
{
   write(2, "**** SEGMENTATION FAULT ****\n", 29);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SEGV'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11);
}
#endif

EAPI void
e_sigill_act(int x, siginfo_t *info, void *data)
{
   write(2, "**** ILLEGAL INSTRUCTION ****\n", 30);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SIGILL'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11);
}

EAPI void
e_sigfpe_act(int x, siginfo_t *info, void *data)
{
   write(2, "**** FLOATING POINT EXCEPTION ****\n", 35);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SIGFPE'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11);
}

EAPI void
e_sigbus_act(int x, siginfo_t *info, void *data)
{
   write(2, "**** BUS ERROR ****\n", 21);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SIGILL'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11);
}

EAPI void
e_sigabrt_act(int x, siginfo_t *info, void *data)
{
   write(2, "**** ABORT ****\n", 21);
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_ungrab();
   ecore_x_sync();
   e_alert_show("This is very bad. Enlightenment SIGABRT'd.\n"
		"\n"
		"This is not meant to happen and is likely a sign of\n"
		"a bug in Enlightenment or the libraries it relies\n"
		"on. You can gdb attach to this process now to try\n"
		"debug it or you could exit, or just hit restart to\n"
		"try and get your desktop back the way it was.\n"
		"\n"
		"Please compile everything with -g in your CFLAGS\n");
   exit(-11);
}
