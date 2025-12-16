

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/utils/file.h>
// #include <freerdp/utils/sleep.h>
#include <freerdp/utils/memory.h>
// #include <freerdp/utils/thread.h>
#include <freerdp/utils/sh_logger.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <execinfo.h>
//#include <freerdp/type.h>

static int trace_fd;

int trace_open(char *trace_marker_abs_path)
{
  trace_fd = open(trace_marker_abs_path, O_WRONLY);
  return trace_fd;
}

int trace_close(void)
{
  close(trace_fd);
}

void trace_write(const char *fmt, ...)
{
        va_list ap;
        char buf[256];
        int n;

        if (trace_fd < 0)
                return;

        va_start(ap, fmt);
        n = vsnprintf(buf, 256, fmt, ap);
        va_end(ap);

        write(trace_fd, buf, n);
}

//On ARM application must be compiled with: -funwind-tables
void log_print_trace (void)
{
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  fprintf (stderr,"Crash Report\n");
  fprintf (stderr,"Obtained %zd stack frames.\n", size);
  for (i = 0; i < size; i++)
     fprintf (stderr,"%s\n", strings[i]);
  free (strings);
}

void sh_start_logger(shDataLogger * logger)
{
	if(logger->filename)
	{
	   logger->fileptr  = fopen(logger->filename,"a+");
	}
}

void sh_logger_free(shDataLogger * logger)
{
	if (logger != NULL)
	{
		xfree(logger,__func__);
	}
}

shDataLogger* sh_logger_new(const char * filename)
{
	shDataLogger* logger = NULL;

	logger = (shDataLogger*) xzalloc(sizeof(shDataLogger),__func__);

	if (logger != NULL)
	{
		logger->filename = filename;
	}

	return logger;
}

//generates a normally distributed Random Number
unsigned int sh_random_in_range (unsigned int min, unsigned int max)
{
  int base_random = rand(); /* in [0, RAND_MAX] */
  if (RAND_MAX == base_random) return sh_random_in_range(min, max);
  /* now guaranteed to be in [0, RAND_MAX) */
  int range       = max - min,
      remainder   = RAND_MAX % range,
      bucket      = RAND_MAX / range;
  /* There are range buckets, plus one smaller interval
     within remainder of RAND_MAX */
  if (base_random < RAND_MAX - remainder) {
    return min + base_random/bucket;
  } else {
    return sh_random_in_range (min, max);
  }
}

/* get time in milliseconds */
inline unsigned int sh_log_get_mstime(void)
{
	struct timeval tp;

	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

/* get time in milliseconds */
INT32 sh_log_get_msinterval(UINT32 time1,UINT32 time2)
{
	return (INT32)(time1 - time2);
}


void sh_syslog (int level, const char *format, ...)
{
    va_list args;
    va_start (args, format);
	vsyslog(level, format, args);
	if(level == LOG_ERR)
		vprintf(format, args);
    va_end(args);
    if(level == LOG_ERR)
    	printf("\n");
}

void sh_log(shDataLogger * logger, char *data, ...)
{
   va_list pl;

   if(logger->fileptr != NULL)
   {
	 //fprintf(logger->fileptr,"%u : ",sh_log_get_mstime());
     va_start (pl, data);
     vfprintf(logger->fileptr,data,pl);
     va_end(pl);
     fflush (logger->fileptr);
   }
   else
      fprintf(stderr,"LogFile: Failed to Open File\n");
}

//On ARM application must be compiled with: -funwind-tables
void sh_log_print_trace (void)
{
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  fprintf (stderr,"Crash Report\n");
  fprintf (stderr,"Obtained %zd stack frames.\n", size);
  for (i = 0; i < size; i++)
     fprintf (stderr,"%s\n", strings[i]);
  free (strings);
}

void sh_stop_logger(shDataLogger * logger)
{
	if(logger)
	{
		if(logger->fileptr)
		{
		   fclose(logger->fileptr);
		   logger->fileptr = NULL;
		}
	}
}
