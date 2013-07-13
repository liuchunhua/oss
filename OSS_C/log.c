
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>


#include "log.h"

static FILE *logfile;

FILE *log_open(){

	logfile = fopen("hello.log","w");
	if(logfile == NULL){
		perror("logfile");
		exit(EXIT_FAILURE);
	}

	setlinebuf(logfile);
	return logfile;
}

void log_close()
{
	fclose(logfile);	
}
void log_msg(const char *format, ...)
{
	va_list op;
	va_start(op,format);
	vfprintf(logfile,format,op);
}
void log_stat(struct stat *stat){
	//dev_t
	log_struct(stat, st_dev, %lld, );

	//ino_t
	log_struct(stat, st_ino, %lld, );
	
	//mode_t
	log_struct(stat, st_mode, 0%o, );

	//nlink_t
	log_struct(stat, st_nlink, %d, );

	//uid_t
	log_struct(stat, st_uid, %d, );

	//gid_t
	log_struct(stat, st_gid, %d, );

	//dev_t
	log_struct(stat, st_rdev, %lld, );

	//off_t
	log_struct(stat, st_size, %lld, );

	//blksize_t
	log_struct(stat, st_blksize, %lld, );

	//blkcnt_t
	log_struct(stat, st_blocks, %lld, );

	//time_t
	log_struct(stat, st_atime, 0x%08lx, );
	log_struct(stat, st_mtime, 0x%08lx, );
	log_struct(stat, st_ctime, 0x%08lx, );
}


void logger_debug(const char *format, ...)
{
	if(Logger.level == 0){
		va_list op;
		va_start(op, format);
		fprintf(logfile, "[DEBUG] %s:%d",__FILE__, __LINE__);
		vfprintf(logfile, format, op);
		fprintf(logfile,"\n");
	}
}


void logger_info(const char *format, ...)
{
	if(Logger.level <= 1){
		va_list op;
		va_start(op, format);
		fprintf(logfile, "[INFO] %s:%d",__FILE__, __LINE__);
		vfprintf(logfile, format, op);
		fprintf(logfile,"\n");
	}
}

void logger_warn(const char *format, ...)
{
	if(Logger.level <= 2)
	{
		va_list op;
		va_start(op, format);
		fprintf(logfile, "[WARN] %s:%d",__FILE__, __LINE__);
		vfprintf(logfile, format, op);
		fprintf(logfile,"\n");
	}
}


void logger_error(const char *format, ...)
{
	if(Logger.level <= 3)
	{
		va_list op;
		va_start(op, format);
		fprintf(logfile, "[ERROR] %s:%d",__FILE__, __LINE__);
		vfprintf(logfile, format, op);
		fprintf(logfile,"\n");
	}
}
