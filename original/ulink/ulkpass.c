/************************************************************************
*									*
*	Copyright (C) 1982,1985,1987 by Unidot, Inc.			*
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
*			ulkpass.c - pass processing			*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: ulkpass.c,v 4.3 92/04/26 16:13:33 rmm Rel $ pass processing";

#include "ulink.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
/*
 * dopass - Performs a single pass over all of the object and library
 * files.
 */

dopass( filex, files ) int filex; char **files;{


	reg char	*sp;
	reg int		i;
	char		line[128];

	if( OVLYFILE ){		/* building an overlay */
		ovlnum = 0;
		fseek( OVLYFILE, 0L, 0 );
		for(;;){
			sp = line;
			while( (i = getc(OVLYFILE)) != '\n' ){
				if( i == EOF ) return;
				*sp++ = i;
			}
			*sp = 0;
			sp = line;
			while( *sp == ' ' ) sp++;
			if( *sp == 0 ) continue;
			if( *sp == '%' ) ovlctl( sp ); else dofile( sp );
		}
	}
	for( i=0; i<filex; i++ ) dofile( files[i] );
}

dofile(s) char *s;{

	reg char	*sp;
	reg int		i;

	curfile = s;
#ifdef STATS
#ifdef DEBUG
prstats("start of dofile");
#endif
#endif
	if( (OBJIN = fopen( curfile, "r" )) == NULL ){
		error( "45 Cannot open %s", curfile );
		return;
	}
#ifdef STATS
	if( inbuf == NULL ) inbuf = palloc( BUFSIZ, BUFUSE );
#else
	if( inbuf == NULL ) inbuf = palloc( BUFSIZ );
#endif
	setbuf( OBJIN, inbuf);

	sp = lastcomp( curfile );
	strcpy( curmod, sp );	/* default module name */
	i = ofill(&objblk, OBJIN );
	if( i == -1 ) error( "F24 Empty file %s",curfile);
	if( i == UOBLST )
		library();
	else
		object();
	curmod[0] = 0;
	fclose( OBJIN );
}

ovlctl(s) char *s; {

	reg int		i;
	reg char	*p;
	reg SECTION	*sep;
	reg int		j;

	if( afmt ) error("F18 no overlay in a.out format");
	s++;
	i = 0;
	if( strncmp( s, "overlay", 7 ) == 0 ){
		s += 7;
		while( *s == ' ' ) s++;
		while( *s >= '0' && *s <= '9' ) i = i*10 + *s++ - '0';
		if( i < 1 || i > 255 )
			error("F26 Bad overlay number: %d",i);
	}
	ovlnum = i;
	for(;;){
		while( *s == ' ' ) s++;
		if( *s == 0 ){
			if( i && pass2 ){		/* new overlay */
				sep = selook( "text" );	/* KLUDGE */
				objblk.ob_type = 0;
				oflush();
				objblk.ob_type = UOBOVL;
				oputb( i );
				oputl( sep->se_val );
				oflush();
			}
			return;
		}
		p = s;
		while( *p && *p != ' ' && *p != ',' ) p++;
		j = *p;
		*p = 0;
		locspec( s );
		*p = j;
		if( j == ',' ) p++;
		s = p;
	}
}
/*
 * library - Processes a single library file.  Expects objbuf to contain
 * the library start block.
 */

library(){		/* simply reads OBJIN - RMM */

	long		off;
	reg char	*sym;
	reg SYTAB	*stp;
	char		changed;
	reg int		i;
	long		libpos;

	do {
		changed = 0;
		fseek( OBJIN, 0L, 0 );	/* M000 didn't have third field */
		/* first read library start block */
		if( ofill(&libblk, OBJIN ) != UOBLST )
			error("F19 Bad library read");
		while( (i = ofill(&libblk, OBJIN )) == UOBLIX ){
			off = ogetl(&libblk );		/* obj module offset */
			ogetl(&libblk );		/* obj module size */
			strcpy( curmod, ogets(&libblk )); /* obj module name */
			while( libblk.ob_ptr < libblk.ob_top ){
				sym = ogets(&libblk );
				stp = sypeek( sym );
				if( stp && (pass2 ?
				    stp->sy_atr & SAUP2 :
				    stp->sy_typ == STUND)){
					libpos = ftell( OBJIN );
					fseek( OBJIN, off, 0 );
					ofill(&objblk, OBJIN );
					object();
					changed = 1;
					fseek( OBJIN, libpos, 0 );
					break;
				}
			}
		}
		if( i == -1 ) error("F20 Bad library end");
	} while( changed );
}
