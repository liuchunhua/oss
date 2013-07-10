#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG(F,...) \
		log_msg("[LOG] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);
#define log_struct(st, field, format, typecast)	\
		log_msg("%s:%5d> " #field " = " #format "\n", \
		__FILE__, __LINE__, typecast st->field)

FILE *log_open(void);
void log_close();

void log_msg(const char *format, ...);
void log_stat(struct stat *stat);

#endif
