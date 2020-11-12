#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

noreturn void exit_with_error(const char* restrict format, ...) {
   va_list arglist;
   va_start( arglist, format );
   vfprintf(stderr, format, arglist);
   va_end( arglist );
   exit(-1);
}