#ifndef __SH_LOGGER_H
#define __SH_LOGGER_H

typedef struct sh_data_logger shDataLogger;

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
struct sh_data_logger
{
	FILE * fileptr;
	//pthread_mutex_t mutex;
	const char * filename;
};

void sh_start_logger(shDataLogger * logger);
void sh_stop_logger(shDataLogger * logger);
void sh_log_print_trace (void);
void sh_log(shDataLogger * logger, char *data, ...);
shDataLogger* sh_logger_new(const char * filename);
unsigned int sh_random_in_range (unsigned int min, unsigned int max);
void sh_logger_free(shDataLogger * logger);
int sh_log_get_msinterval(unsigned int time1,unsigned int time2);
unsigned int sh_log_get_mstime(void);
void log_print_trace (void);
void sh_syslog (int level, const char *format, ...);
//On ARM application must be compiled with: -funwind-tables for the trace to work

int trace_open(char *trace_marker_abs_path);
int trace_close(void);
void trace_write(const char *fmt, ...);

#endif
