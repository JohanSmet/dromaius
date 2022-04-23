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
#ifdef PLATFORM_WINDOWS
	#define	dms_strlen		strlen
	#define	dms_strcmp		strcmp
	#define dms_strlcpy(dst,src,dsize)	strcpy_s(dst,dsize,src)
	#define dms_strdup		_strdup
	#define dms_strlcat(dst,src,dsize)	strcat_s(dst,dsize,src)
	#define	dms_strncmp		strncmp
#else
	#define	dms_strlen		strlen
	#define	dms_strcmp		strcmp
	#define dms_strlcpy(dst,src,dsize)	strcpy(dst,src)		/* FIXME */
	#define dms_strdup		strdup
	#define dms_strlcat(dst,src,dsize)	strcat(dst,src)		/* FIXME */
	#define	dms_strncmp		strncmp
#endif

#define dms_snprintf	snprintf
#define dms_vsnprintf	vsnprintf

// file
#ifdef PLATFORM_WINDOWS
	#define dms_fopen(fp,name,mode)		fopen_s(&fp,name,mode)
	#define dms_fclose					fclose
	#define dms_fread					fread
	#define dms_fwrite					fwrite
	#define dms_feof					feof
#else
	#define dms_fopen(fp,name,mode)		((fp = fopen(name,mode)) == NULL)
	#define dms_fclose					fclose
	#define dms_fread					fread
	#define dms_fwrite					fwrite
	#define dms_feof					feof
#endif





#endif // DROMAIUS_CRT_H
