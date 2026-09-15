#ifndef INCLUDE_TYPES_H
#define INCLUDE_TYPES_H

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;

#ifndef __SIZE_TYPE__
typedef unsigned int size_t;
#else
typedef __SIZE_TYPE__ size_t;
#endif

#define NULL ((void *)0)

#endif /* INCLUDE_TYPES_H */
