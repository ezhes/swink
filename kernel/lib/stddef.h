#ifndef LIB_STDDEF
#define LIB_STDDEF

#define NULL ((void*)0)
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE*)0)->MEMBER)
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;
typedef unsigned long long uintptr_t;
#endif /* LIB_STDDEF */