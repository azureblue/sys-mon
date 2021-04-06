#include <stdnoreturn.h>

noreturn void exit_with_error(const char* format, ...);
noreturn void exit_with_perror(const char* restrict msg_format, ...);

