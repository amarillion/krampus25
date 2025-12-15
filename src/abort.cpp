#include "abort.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

void abort_example(char const *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}
