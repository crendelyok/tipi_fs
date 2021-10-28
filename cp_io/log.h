#ifndef MY_LOG_H
#define MY_LOG_H

#include <stdio.h>
#include <errno.h>

#define LOG(format, ...)                  \
do {                                      \
	perror(format, ##__VA_ARGS__);    \
} while(0)                                

#endif
