/*
 * FreeRDP: A Remote Desktop Protocol Client
 * Memory Utils
 *
 * Copyright 2001-2011 Jay Sorg, significant updates and ammendments including leak detection by John O'Sullivan@Cloudium 2014
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <syslog.h>

#include <freerdp/utils/memory.h>
#include <freerdp/utils/file.h>
#include <freerdp/utils/sh_logger.h>
#include <linux/limits.h>
#include <sys/statvfs.h>
#include <math.h>       /* ceil */



/*
 *
 * How To use
 * To enable define MEMORY_ALLOCATION_MONITOR
 * When the program exits normally we should get a dump of memory which has not been freed
 * Turning on Memory Monitoring Dynamically
 * To create a dump of allocated memory just create a file ML in /usr/local
 * This will populate a file called allocation.txt (in /usr/local)
 * You can view this file to see realtime memory allocation and de-allocation
 */

//#define DEBUG_TRACK_LOCK

#define EOS      '\0'              /* End of string sentinel */
#define V	 (void)
#define sm_min(a, b) ((a) < (b) ? (a) : (b))

#define PATTERN 0xA
#define FILL_PATTERN 0x55
#define MARGIN 0 //must be a multiple of 4


pthread_mutex_t track_lock;

typedef struct track_t {
   char *data;
   unsigned long space;
   unsigned allocated_at;
   char function_reference[250];
   struct track_t *next;
} track_t;

static track_t *track = NULL;
static unsigned long allocated = 0;
static int dump_flag = 1;

FILE * fd_debug;

int dump_allocated_space();
int show_allocated_space();

int init_memory_leak_monitor()
{
	if (pthread_mutex_init(&track_lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 0;
	}
	fd_debug = fopen("/usr/local/alloc_dealloc.txt","w");
	return 1;
}

void deinit_memory_leak_monitor()
{
	pthread_mutex_destroy(&track_lock);
	fclose(fd_debug);
}

//#define DEBUG_MLA_VERBOSE


void *smartcalloc(unsigned long bytes, const char *file, int line, char fill)
{
   track_t *temp;
   char *data;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_lock\n",__func__);
#endif
   pthread_mutex_lock(&track_lock);
   if ((temp = (track_t *) calloc(1,sizeof(track_t))) == NULL ||
    (data = calloc(1,bytes + 2*MARGIN)) == NULL) {
      fprintf(stdout, "Calloc failure in file %s on line %d\n", file, line);
      exit(1);
   }
   data += MARGIN;
   temp->data = data;
   //corrib_syslog(LOG_DEBUG,"temp->function_reference = %p, file = %p\n",temp->function_reference,file);
   //////////////////////////////////////////////////// ARPM: uncomment!! su_strlcpy(temp->function_reference,file,250);
   temp->allocated_at = sh_log_get_mstime();
#ifdef FILL_MEMORY
   memset(data, fill, bytes);
#endif
   allocated += bytes;
   if(MARGIN)
   {
	   memset(data-MARGIN, PATTERN, MARGIN);
	   memset(data+bytes, PATTERN, MARGIN);
   }
   temp->space = bytes;

   temp->next = track;
#ifdef DEBUG_MLA_VERBOSE
   corrib_syslog(LOG_DEBUG,"%s:data =%p, function:%s,space=%lu\n",__func__,temp->data,temp->function_reference,temp->space);
#endif
   track = temp;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_unlock\n",__func__);
#endif

   pthread_mutex_unlock(&track_lock);
   if(freerdp_check_file_exists("/usr/local/ML"))
   {
	   unlink("/usr/local/ML");
	   dump_allocated_space();

   }
   return data;
}


void *smartalloc(unsigned long bytes, const char *file, int line, char fill)
{
   track_t *temp;
   char *data;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_lock\n",__func__);
#endif
   pthread_mutex_lock(&track_lock);
   if ((temp = (track_t *) malloc(sizeof(track_t))) == NULL ||
    (data = malloc(bytes + 2*MARGIN)) == NULL) {
      fprintf(stdout, "Malloc failure in file %s on line %d\n", file, line);
      exit(1);
   }
   data += MARGIN;
   temp->data = data;
   strcpy(temp->function_reference,file);
   temp->allocated_at = sh_log_get_mstime();
#ifdef FILL_MEMORY
   memset(data, fill, bytes);
#endif
   allocated += bytes;
   if(MARGIN)
   {
	   memset(data-MARGIN, PATTERN, MARGIN);
	   memset(data+bytes, PATTERN, MARGIN);
   }
   temp->space = bytes;

   temp->next = track;
#ifdef DEBUG_MLA_VERBOSE
   corrib_syslog(LOG_DEBUG,"%s:data =%p, function:%s,space=%lu\n",__func__,temp->data,temp->function_reference,temp->space);
#endif
   track = temp;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_unlock\n",__func__);
#endif
   pthread_mutex_unlock(&track_lock);
   if(freerdp_check_file_exists("/usr/local/ML") && (dump_flag == 1)) //to snapshot dumps do a ML followed by a DA, the repeat the sequence
   {
	   unlink("/usr/local/ML");
	   dump_allocated_space();
	   dump_flag =1;
   }
   /*
   if(freerdp_check_file_exists("/usr/local/DA") && ( dump_flag == 0))
   {
	   corrib_syslog(LOG_DEBUG,"Resetting dump flag\n");
	   dump_flag =1;
   }
   */
   if(freerdp_check_file_exists("/usr/local/AL"))
	   corrib_syslog(LOG_DEBUG,"Total Allocated = %lu\n",allocated);
   return data;
}

int show_allocated_space()
{
   track_t *temp;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_lock\n",__func__);
#endif
   pthread_mutex_lock(&track_lock);
   if (track == NULL)
   {
	  fprintf(stdout,"No space currently allocated\n");
	  pthread_mutex_unlock(&track_lock);
	  return -1;
   }
   corrib_syslog(LOG_DEBUG,"Allocated Space\n-------------------------\n");
   temp = track;
   while(temp != NULL)
   {
	   //corrib_syslog(LOG_DEBUG,"%p->%p : %ld :%s\n",temp,temp->data,temp->space,temp->function_reference);
	   corrib_syslog(LOG_DEBUG,"%u:%p:%lu bytes,%s\n",temp->allocated_at,temp->data,temp->space,temp->function_reference);
	   temp = temp->next;
   }
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_unlock\n",__func__);
#endif
   pthread_mutex_unlock(&track_lock);
   return 1;

}


int dump_allocated_space()
{
   track_t *temp;
   unsigned int counter = 0;
   pthread_mutex_lock(&track_lock);
   corrib_syslog(LOG_DEBUG,"Dumping Allocated Space\n");
   FILE * fd = fopen("/usr/local/allocation.txt","w");
   if(fd != NULL)
   {
	   if (track == NULL)
	   {
		  fprintf(fd,"No space currently allocated\n");
		  fclose(fd);
		  return -1;
	   }
	   fprintf(fd,"Allocated Space\n-------------------------\n");
	   temp = track;
	   while(temp != NULL)
	   {
		   fprintf(fd,"%u:%p->%p : %ld :%s\n",temp->allocated_at,temp,temp->data,temp->space,temp->function_reference);
		   temp = temp->next;
	   }
	   fflush(fd);
	   fclose(fd);
   }
   else
	   corrib_syslog(LOG_DEBUG,"Could not open file in /usr/local\n");
   pthread_mutex_unlock(&track_lock);

   return 1;

}


/*
 * Not working at the moment
 */
int get_allocated_space(void *address,const char * file,int line)
{
   track_t *temp, *to_free;
   pthread_mutex_lock(&track_lock);
   if (track == NULL) {
	  fprintf(stdout,"%s:Failed to determine non-malloced space in file %s at line %d\n",__func__,file, line);
	  return -1;
   }
   if (track->data == address)
   {
	  to_free = track;
   }
   else
   {
	  for (temp = track; temp->next != NULL && temp->next->data != address;)
		 temp = temp->next;
	  if (temp->next == NULL)
	  {
		 fprintf(stdout,"Could not find allocated space for re-alloc in file %s at line %d\n", file, line);
		 return -1;
	  }
	  to_free = temp->next;
   }
   pthread_mutex_unlock(&track_lock);
   return to_free->space;

}

track_t * remove_list_entry(void *address,const char *file)
{
   track_t *temp=NULL, *to_free=NULL, *prev=NULL;
   int i;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_lock\n",__func__);
#endif

   if (track == NULL) {
      fprintf(stdout,"%s:1. Attempt to free non-malloced space in file %s\n",__func__,file);
      log_print_trace();

      return;
   }
   if (track->data == address) {
      to_free = track;
      track = track->next;
   }
   else
   {

	  temp = track;
	  while(temp != NULL)
	  {

		  //corrib_syslog(LOG_DEBUG,"temp is %p->%p  address is %p\n",temp,temp->data,address);
		  if(temp->data == address)
		  {
			  //corrib_syslog(LOG_DEBUG,"found!!!!!!!!!!!\n");
			  break;
		  }
		  prev = temp;
		  temp = temp->next;
	  }
      if (temp == NULL) { //if we cannot find it return an error
         //fprintf(stdout,"%s:2. Attempt to free non-malloced space in file %s at line %d for %p\n",__func__,file, line,address);
   	   	 //log_print_trace();

         fprintf(stdout,"%s:1. Error finding list entry in file %s\n",__func__,file);
         return NULL;
      }
      if((prev == NULL) && (temp != NULL)) //head of the list
      {
    	  to_free = temp;
    	  track = to_free->next;
      }
      if((prev != NULL) && (temp != NULL))
      {
    	  to_free = temp;
    	  prev->next = temp->next; //remove it from the list
      }
   }
   return to_free;

}


void smartfree(void *address, const char *file, int line)
{
   track_t *temp=NULL, *to_free=NULL, *prev=NULL;
   int i;
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_lock\n",__func__);
#endif
   pthread_mutex_lock(&track_lock);
   if (track == NULL) {
      fprintf(stdout,"%s:1. Attempt to free non-malloced space in file %s at line %d\n",__func__,file, line);
      log_print_trace();
#ifdef DEBUG_TRACK_LOCK
   corrib_syslog(LOG_DEBUG,"%s:track_unlock\n",__func__);
#endif
      pthread_mutex_unlock(&track_lock);
      return;
   }
   to_free = remove_list_entry(address,file);
   if(to_free)
   {
	   for (i = 0; i < MARGIN; i++)
		  if (to_free->data[to_free->space + i] != PATTERN ||
		   to_free->data[-MARGIN + i] != PATTERN) {
			 fprintf(stdout,"%s: Space freed in file %s at line %d has data written past bounds.\n",__func__,file, line);
			 break;
		  }
	#ifdef FILL_MEMORY
	   memset(to_free->data, PATTERN, to_free->space);
	#endif
	   allocated -= to_free->space;
	#ifdef DEBUG_MLA_VERBOSE
	   corrib_syslog(LOG_DEBUG,"%s: Freeing %lu bytes at %p for %s\n",__func__,to_free->space,(void *)(to_free->data),to_free->function_reference);
	#endif
	   free(to_free->data - MARGIN);
	   free(to_free);
	#ifdef DEBUG_TRACK_LOCK
	   corrib_syslog(LOG_DEBUG,"%s:track_unlock\n",__func__);
	#endif
   }
   else
   {
	   corrib_syslog(LOG_DEBUG,"%s: Failed to find entry for address %p in %s\n",__func__,address,file);
	   exit(0);
   }
   pthread_mutex_unlock(&track_lock);
}


void *smartrealloc(const char * function_name, int lineno, void * ptr, unsigned  size)
{
	unsigned osize = size;
	void *buf;

	assert(size > 0);

	/*  If	the  old  block  pointer  is  NULL, treat realloc() as a
	   malloc().  SVID is silent  on  this,  but  many  C  libraries
	   permit this.  */

	if (ptr == NULL)
	   return smartalloc(size, function_name, lineno, FILL_PATTERN);

	osize = get_allocated_space(ptr,function_name,lineno);
	if( osize < 0)
	{
        corrib_syslog(LOG_DEBUG,"%s:Attempt to free non-malloced space in file %s at line %d\n",__func__,function_name,lineno);
        assert(0);
	}
	else
	{
		//corrib_syslog(LOG_DEBUG,"osize = %d\n",osize);
	}


	if ((buf = smartalloc(size, function_name, lineno, FILL_PATTERN)) != NULL)
	{
	   V memcpy(buf, ptr, (int) sm_min(size, osize));
	   /* If the new buffer is larger than the old, fill the balance
              of it with "designer garbage". */
	   if (size > osize) {
#ifdef FILL_MEMORY
	      V memset(((char *) buf) + osize, 0x55, (int) (size - osize));
#endif
	   }

	   /* All done.  Free and dechain the original buffer. */
	   //corrib_syslog(LOG_DEBUG,"%s:Freeing original ptr %p\n",__func__,ptr);
	   smartfree(ptr,function_name,1);
	}
	return buf;
}


char * smartstrdup (const char *s,const char * function_name) {
    char *d = smartalloc (strlen (s) + 1,function_name, 1,0);   // Allocate memory
    if (d != NULL) strcpy (d,s);         // Copy string if okay

	//corrib_syslog(LOG_DEBUG,"Allocated %d bytes at %p in %s\n",strlen(s),d,__func__);
    return d;                            // Return new memory
}

/**
 * Allocate memory.
 * This function is used to secure a malloc call.
 * It verifies its return value, and logs an error if the allocation failed.
 *
 * @param size - number of bytes to allocate. If the size is < 1, it will default to 1.
 *
 * @return a pointer to the allocated buffer. NULL if the allocation failed.
 */

void* xmalloc(size_t size,const char * function_name)
{
	void* mem;

	if (size < 1)
		size = 1;
#ifdef MEMORY_ALLOCATION_MONITOR
	mem=smartalloc(size, function_name, 1, FILL_PATTERN);
#else
	mem = malloc(size); //memory is not initialised
#endif
#ifdef MEMORY_LEAK_DEBUG
	//if(freerdp_check_file_exists("/usr/local/ML"))
		fprintf(fd_debug,"A %d to %s:%p\n", size,function_name,mem);
#endif
	if (mem == NULL)
	{
		perror("xmalloc");
		printf("xmalloc: failed to allocate memory of size: %d\n", (int) size);
	}

	return mem;
}

/**
 * Allocate memory initialized to zero.
 * This function is used to secure a calloc call.
 * It verifies its return value, and logs an error if the allocation failed.
 *
 * @param size - number of bytes to allocate. If the size is < 1, it will default to 1.
 *
 * @return a pointer to the allocated and zeroed buffer. NULL if the allocation failed.
 */
void* xzalloc(size_t size,const char * function_name)
{
	void* mem;

	if (size < 1)
		size = 1;
#ifdef MEMORY_ALLOCATION_MONITOR
	mem=smartcalloc(size, function_name, 1, 0);
#else
	mem = calloc(1, size); //memory is initialized to zero
#endif

#ifdef MEMORY_LEAK_DEBUG
	//if(freerdp_check_file_exists("/usr/local/ML"))
		fprintf(fd_debug,"A %d to %s:%p\n", size,function_name,mem);
#endif
	if (mem == NULL)
	{
		perror("xzalloc");
		printf("xzalloc: failed to allocate memory of size: %d\n", (int) size);
	}

	return mem;
}

/**
 * Reallocate memory.
 * This function is used to secure a realloc call.
 * It verifies its return value, and logs an error if the allocation failed.
 *
 * @param ptr - pointer to the buffer that needs reallocation. This can be NULL, in which case a new buffer is allocated.
 * @param size - number of bytes to allocate. If the size is < 1, it will default to 1.
 *
 * @return a pointer to the reallocated buffer. NULL if the allocation failed (in which case the 'ptr' argument is untouched).
 */

void* xrealloc(void* ptr, size_t size,const char * function_name)
{
	void* mem;

	if (size < 1)
		size = 1;
#ifdef MEMORY_ALLOCATION_MONITOR
	mem=smartrealloc(function_name, 1,ptr, size);
#else
	mem = realloc(ptr, size);
#endif
	//printf("A %d to %pS\n",size, __builtin_return_address(0));

	if (mem == NULL)
		perror("xrealloc");
#ifdef MEMORY_LEAK_DEBUG
	//if(freerdp_check_file_exists("/usr/local/ML"))
		fprintf(fd_debug,"R:%d to %s:%p\n",size,function_name,ptr);
#endif
	return mem;
}

/**
 * Free memory.
 * This function is used to secure a free call.
 * It verifies that the pointer is valid (non-NULL) before trying to deallocate it's buffer.
 *
 * @param ptr - pointer to a buffer that needs deallocation. If ptr is NULL, nothing will be done (no segfault).
 */

void xfree(void* ptr,const char * function_name)
{
	if (ptr != NULL)
	{
#ifdef MEMORY_ALLOCATION_MONITOR
	smartfree(ptr,function_name,1);
#else
#ifdef MEMORY_LEAK_DEBUG
		//if(freerdp_check_file_exists("/usr/local/ML"))
			fprintf(fd_debug,"F:from %s:%p\n",function_name,ptr);
			fflush(fd_debug);
#endif
	free(ptr);
#endif

		//printf("F from %pS\n", __builtin_return_address(0));

	}
}

char * xgetcwd(char *buf, size_t size,char * function_name)
{
	if(buf == NULL)
	{

	    char *d = smartalloc (PATH_MAX,function_name, 1,0);   // Allocate memory
	    if (d != NULL)
	    {
	    	getcwd(d, 0);
	    }
	    return d;
	}
	else
		return getcwd(buf, 0);

}

/**
 * Duplicate a string in memory.
 * This function is used to secure the strdup function.
 * It will allocate a new memory buffer and copy the string content in it.
 * If allocation fails, it will log an error.
 *
 * @param str - pointer to the character string to copy. If str is NULL, nothing is done.
 *
 * @return a pointer to a newly allocated character string containing the same bytes as str.
 * NULL if an allocation error occurred, or if the str parameter was NULL.
 */

char* xstrdup(const char* str)
{
	char* mem;

	if (str == NULL)
		return NULL;

#ifdef _WIN32
	mem = _strdup(str);
#else
#ifdef MEMORY_ALLOCATION_MONITOR
	mem = smartstrdup(str,__func__);
#else

	mem = strdup(str);
#endif
#endif

	if (mem == NULL)
		perror("strdup");


	return mem;
}

/**
 * Duplicate a wide string in memory.
 * This function is used to secure a call to wcsdup.
 * It verifies the return value, and logs a message if an allocation error occurred.
 *
 * @param wstr - pointer to the wide-character string to duplicate. If wstr is NULL, nothing will be done.
 *
 * @return a pointer to the newly allocated string, containing the same data as wstr.
 * NULL if an allocation error occurred (or if wstr was NULL).
 */

wchar_t* xwcsdup(const wchar_t* wstr)
{
	wchar_t* mem;

	if (wstr == NULL)
		return NULL;

#ifdef _WIN32
	mem = _wcsdup(wstr);
#elif sun
	mem = wsdup(wstr);
#elif (defined(__APPLE__) && defined(__MACH__)) || defined(ANDROID)
	mem = xmalloc(wcslen(wstr));
	if (mem != NULL)
		wcscpy(mem, wstr);
#else
	mem = wcsdup(wstr);
#endif

	if (mem == NULL)
		perror("wstrdup");

	return mem;
}

/**
 * Create an uppercase version of the given string.
 * This function will duplicate the string (using xstrdup()) and change its content to all uppercase.
 * The original string is untouched.
 *
 * @param str - pointer to the character string to convert. This content is untouched by the function.
 *
 * @return pointer to a newly allocated character string, containing the same content as str, converted to uppercase.
 * NULL if an allocation error occured.
 */
char* xstrtoup(const char* str)
{
	char* out;
	char* p;
	int c;
	out = xstrdup(str);
	if(out != NULL)
	{
		p = out;
		while(*p != '\0')
		{
			c = toupper((unsigned char)*p);
			*p++ = (char)c;
		}
	}
	return out;
}
