#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

noreturn void exit_with_error(const char* restrict format, ...) {
   va_list arglist;
   va_start( arglist, format );
   vfprintf(stderr, format, arglist);
   fprintf(stderr, "\n");
   va_end( arglist );
   exit(-1);
}

noreturn void exit_with_perror(const char* restrict msg_format, ...) {
   va_list arglist;
   va_start( arglist, msg_format );
   vfprintf(stderr, msg_format, arglist);
   fprintf(stderr, ":");
   va_end( arglist );
   perror(NULL);
   exit(-1);
}

