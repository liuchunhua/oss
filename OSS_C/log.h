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
#define log_debug(F, ...) \
		log_msg("[DEBUG] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);

#define log_info(F, ...) \
		log_msg("[INFO] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);

#define log_warn(F, ...) \
		log_msg("[WARN] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);

#define log_error(F, ...) \
		log_msg("[ERROR] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);

typedef struct{
	int level;
	void (*debug)(const char *, ...);
	void (*info)(const char *, ...);
	void (*warn)(const char *, ...);
	void (*error)(const char *, ...);
}LoggerOpration;

void logger_debug(const char *format, ...);
void logger_info(const char *format, ...);
void logger_warn(const char *format, ...);
void logger_error(const char *format, ...);


LoggerOpration Logger = {
	.level = 0,
	.debug = logger_debug,
	.info = logger_info,
	.warn = logger_warn,
	.error = logger_error
};

FILE *log_open(void);
void log_close();

void log_msg(const char *format, ...);
void log_stat(struct stat *stat);

#endif
