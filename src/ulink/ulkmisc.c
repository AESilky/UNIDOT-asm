/************************************************************************
*									*
*	Copyright (C) 1982,1985, by Unidot, Inc.			*
*									*
*			Unidot, Inc.					*
*			602 Park Point Dr.				*
*			Golden, CO 80401				*
*									*
*			All Rights Reserved				*
*									*
* This software is furnished under license and may be used and copied	*
* only in accordance with the terms of such license and with the	*
* inclusion of the above copyright notice.  This software or any other	*
* copies thereof may not be provided or otherwise made available to any	*
* other person.								*
*									*
* No title to or ownership of the software is hereby transferred.	*
*									*
* The information in this software is subject to change without notice	*
* and should not be construed as a commitment by Unidot, Inc.		*
*									*
* Unidot assumes no responsibility for the use or reliability of its	*
* software on equipment configurations that are not directly supported	*
* by Unidot, Inc.							*
*									*
*************************************************************************/

/************************************************************************
*									*
*			     UAS Linker					*
*									*
*			ulk.misc.c - miscellaneous routines		*
*									*
************************************************************************/

static char rcsid[]=
"@(#)$Header: ulkmisc.c,v 4.7 92/04/26 16:13:21 rmm Rel $ ulink misc routines";

#include <unistd.h> /* For 'close' and `sbrk` */

#include "ulink.h"
/*
 * error - prints an error message and quits if fatal
 */

error( s,a,b,c,d,e ) reg char *s; long a,b,c,d,e;{

	reg int		flag;
	reg char	*p;
	char		errno[6];

	flag = *s;
	if( flag == 'F' ){
		s++;
		printf( "FATAL: " );
	} else
	if( flag == 'W' ){
		s++;
		printf( "WARNING: " );
	} else
		printf( "ERROR: " );
	p = errno;
	while( *s >= '0' && *s <= '9' ) *p++ = *s++;
	*p = 0;
	while( *s == ' ' ) s++;
	if( errno[0] ) printf("(%s) ",errno);
	if( curmod[0] ) printf("module %s: ",curmod);
	printf( s, a, b, c, d, e );
	printf( "\n" );
	if( flag == 'F' ) quit( FATEXIT );
	if( flag != 'W' ) errct++; else warnct++;
}

nocreat(s) char *s; {		/* cannot create file */

	error("F02 cannot create %s",s);
}

noread(s) char *s; {		/* cannot read file */

	error("F03 file %s does not exist or is unreadable",s);
}
/*
 * palloc - Allocates a block of physical memory of the specified size,
 * and returns a pointer to the block.  The block is guaranteed to be
 * aligned to the coarsest boundary which might be required.
 */

char *
#ifdef STATS
palloc( size, which ) uns size;{
#else
palloc( size ) uns size;{
#endif


	static char	*phytop;
	static char	*phylim;
	reg char	*oldtop;
	reg char	*tmp;
	reg int		i;
//	extern char	*sbrk();

	size = (size + (ALIGN-1)) & -ALIGN;
#ifdef STATS
	usestats[which] += size;
#endif
	if( phytop == 0 ) phytop = phylim = sbrk(0);
	oldtop = phytop;
	phytop += size;
	i = 8192;
	while( phytop > phylim ){
again:		tmp = sbrk(i);
		if( tmp == (char *)0 || tmp == (char *)-1 ){
			i >>= 1;
			if( i < 512 ) error( "F01 out of memory");
			goto again;
		}
		if( tmp != phylim ){	/* not contiguous */
			oldtop = phylim = tmp;
			phytop = oldtop + size;
		}
		phylim += i;
	}
	return oldtop;
}

/*
 * zpalloc() allocates space and guarantees that it is zero
 */

char *
#ifdef STATS
zpalloc( size, which ) uns size; {
#else
zpalloc( size ) uns size; {
#endif

	reg char	*p,*q,*r;

#ifdef STATS
	p = q = palloc( size, which );
#else
	p = q = palloc( size );
#endif
	for( r = q + size; q < r; q++ ) *q = 0;
	return p;
}

/*
 * lastcomp - Returns a pointer to the last component of a path name
 */

char *
lastcomp( s ) reg char *s;{


	reg int		c;
	reg char	*r;

	r = s;
	while( c = *s++ )
		if( c == '/' || c == '\\' || c == ':' || c == ']' )
			r = s;
	return r;
}
