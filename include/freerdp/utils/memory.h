/**
 * FreeRDP: A Remote Desktop Protocol Client
 * Memory Utils
 *
 * Copyright 2009-2011 Jay Sorg
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

#ifndef __MEMORY_UTILS_H
#define __MEMORY_UTILS_H

#include <stddef.h>
#include <ctype.h>
#include <wchar.h>
#include <freerdp/api.h>

//#define MEMORY_ALLOCATION_MONITOR
//#define FILL_MEMORY

//#define MEMORY_LEAK_DEBUG /* turn this on to see interactive allocation and de-allocation, prints to command line so heavy impact on performance */

#ifdef MEMORY_LEAK_DEBUG
	#define MEMORY_ALLOCATION_MONITOR
#endif

FREERDP_API void* xmalloc(size_t size,const char * function_name);
FREERDP_API void* xzalloc(size_t size,const char * function_name);
FREERDP_API void* xrealloc(void* ptr, size_t size,const char * function_name);
FREERDP_API void xfree(void* ptr,const char * function_name);
FREERDP_API char* xstrdup(const char* str);
FREERDP_API char* xstrtoup(const char* str);
FREERDP_API wchar_t* xwcsdup(const wchar_t* wstr);
FREERDP_API int show_allocated_space();
FREERDP_API int dump_allocated_space();
FREERDP_API unsigned long long get_free_ram();
FREERDP_API unsigned long long get_total_ram();
FREERDP_API double get_disk_total();
FREERDP_API double get_disk_free();
char * xgetcwd(char *buf, size_t size,char * function_name);
#define xnew(_type,name) (_type*)xzalloc(sizeof(_type),name)

#define ARRAY_SIZE(_x) (sizeof(_x)/sizeof(*(_x)))

#endif /* __MEMORY_UTILS_H */
