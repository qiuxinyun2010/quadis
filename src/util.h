#include <stddef.h>

#ifndef PSMALLOC
#define PSMALLOC
#include "psmallocv4.h"
#endif

/* API */
#ifdef __cplusplus  
extern "C"
{
#endif

size_t get_file_size(const char* file_path);
void* create_and_map(const char* file_path, size_t page_size);
int check_file_existed(const char* file_path);
ps_ptr ps_stringFromLongLong(long long value);
char *stringFromLongLong(long long value);
#ifdef __cplusplus  
}
#endif