#ifndef MY_LOG_H
#define MY_LOG_H

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#define LOG(format, ...)                  \
do {                                      \
	printf(format, ##__VA_ARGS__);    \
} while(0)                                

#endif
