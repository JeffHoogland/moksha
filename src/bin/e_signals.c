/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* a tricky little devil, requires e and it's libs to be built
 * with the -rdynamic flag to GCC for any sort of decent output. 
 */
void e_sigseg_act(int x, siginfo_t *info, void *data){

  void *array[255];
  size_t size;
  char **strings;
  size_t i;
  write(2, "**** SEGMENTATION FAULT ****\n", 29);
  write(2, "**** Printing Backtrace... *****\n\n", 34);
  size = backtrace (array, 255);
  backtrace_symbols_fd(array, size, 2);
  exit(-11); 
}


     
