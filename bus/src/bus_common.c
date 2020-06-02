#include<stdarg.h>
#include<stdio.h>
#include <syslog.h>

#include "bus_common.h"

void bus_debug(int level, const char* format, ...) {
	va_list args;
	char out[1024];
	va_start(args, format);
	vsprintf(out, format, args);
	printf("%s \n", out);
	syslog(level, "%s", out);
	va_end(args);
}