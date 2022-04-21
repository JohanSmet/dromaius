// crt.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Abstraction layer around C runtime functions

#ifndef DROMAIUS_CRT_H
#define DROMAIUS_CRT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// memory
#define dms_malloc	malloc
#define dms_calloc	calloc
#define dms_free	free

#define dms_zero(ptr,size)			memset(ptr, '\0', size)
#define dms_memset(ptr,c,size)		memset(ptr, (c), size)
#define dms_memcpy(dst,src,size)	memcpy(dst, src, size)
#define dms_memcmp(ptr1,ptr2,num)	memcmp(ptr1, ptr2, num)

// string
#define	dms_strlen		strlen
#define	dms_strcmp		strcmp
#define dms_strcpy		strcpy
#define dms_strncat		strncat
#define	dms_strncmp		strncmp

#define dms_snprintf	snprintf
#define dms_vsnprintf	vsnprintf



#endif // DROMAIUS_CRT_H
