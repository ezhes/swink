#ifndef LIB_STRING
#define LIB_STRING
#include "types.h"

/* Standard. */
void* memcpy(void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
char* strncat(char*, const char*, size_t);
int memcmp(const void*, const void*, size_t);
int strcmp(const char*, const char*);
void* memchr(const void*, int, size_t);
char* strchr(const char*, int);
size_t strcspn(const char*, const char*);
char* strpbrk(const char*, const char*);
char* strrchr(const char*, int);
size_t strspn(const char*, const char*);
char* strstr(const char*, const char*);
void* memset(void*, int, size_t);
size_t strlen(const char*);

/* Extensions. */
size_t strlcpy(char*, const char*, size_t);
size_t strlcat(char*, const char*, size_t);
char* strtok_r(char*, const char*, char**);
size_t strnlen(const char*, size_t);

void *bzero(void *b, size_t size);


#endif /* LIB_STRING */