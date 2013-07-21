#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG(F,...) \
		log_msg("[LOG] %s:%d" F "\n",__FILE__, __LINE__, ##__VA_ARGS__);

#define log_struct(st, field, format, typecast) 	\
		log_msg("%s:%5d> " #field " = " #format "\n", \
		__FILE__, __LINE__, typecast st->field)

#define log_debug(F, ...) \
        Logger.debug("%s:%d:" F, __FILE__, __LINE__, ##__VA_ARGS__);

#define log_info(F, ...) \
		 Logger.info("%s:%d:" F, __FILE__, __LINE__, ##__VA_ARGS__);

#define log_warn(F, ...) \
		 Logger.warn("%s:%d:" F, __FILE__, __LINE__, ##__VA_ARGS__);

#define log_error(F, ...) \
		 Logger.error("%s:%d:" F, __FILE__, __LINE__, ##__VA_ARGS__);

//日志操作
typedef struct
{
    int level;
    FILE *(*open)();
    void (*close)();
    void (*debug)(const char *, ...);
    void (*info)(const char *, ...);
    void (*warn)(const char *, ...);
    void (*error)(const char *, ...);
} LoggerOpration;

void logger_debug(const char *format, ...);
void logger_info(const char *format, ...);
void logger_warn(const char *format, ...);
void logger_error(const char *format, ...);

//日志操作类
extern LoggerOpration Logger;

FILE *log_open(void);
void log_close();

void log_msg(const char *format, ...);

//打印struct stat结构
void log_stat(struct stat *stat);

#endif
