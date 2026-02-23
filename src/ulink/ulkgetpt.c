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
*			ulk.getput.c - general input module		*
*									*
************************************************************************/


static char rcsid[]=
"@(#)$Header: ulkgetpt.c,v 4.4 88/02/25 06:58:27 rmm Rel $ ulink io module";

#include "ulink.h"

/* Definitions (local) */

/*
 * ofill - Reads the next block into the specified object block
 * structure from the specified file.
 */

unsigned m_addr;

char ofill(oblock_t* obp, FILE* fp) {


	obp->ob_type = (char) getc( fp );
	if( (obp->ob_type & 0xff) == 0xff ) return -1;
	obp->ob_top = &obp->ob_buf[getc( fp )&0xff];
	for( obp->ob_ptr = obp->ob_buf;obp->ob_ptr < obp->ob_top;obp->ob_ptr++)
		*obp->ob_ptr = (char) getc( fp );
	obp->ob_ptr = obp->ob_buf;

if( debug > 2 ) {
	printf("ofill: %x: type %d size %x\n",m_addr,obp->ob_type,obp->ob_top - obp->ob_buf);
		m_addr += (obp->ob_top - obp->ob_buf + 2);
	}
	return obp->ob_type;
}

/*
 * oflush - Outputs an object block to the object output file.
 */

void oflush(){

	if( afmt ) return;
	if( objblk.ob_type ){
		putc( objblk.ob_type, OBJOUT );
		putc( objblk.ob_top-objblk.ob_buf, OBJOUT );
		for( objblk.ob_ptr = objblk.ob_buf;
		     objblk.ob_ptr < objblk.ob_top;
		     objblk.ob_ptr++ )
			 putc( *objblk.ob_ptr, OBJOUT );
	}
	objblk.ob_top = objblk.ob_buf;
	objblk.ob_type = 0;
}
/* ogetb - Returns the next byte from the specified object buffer.  */
byte ogetb(oblock_t* obp) {


	return olodb(obp->ob_ptr++);
}

/* ogetl - Returns the next long word from the specified object buffer.  */
long ogetl(oblock_t* obp) {

	long	l;

	l = olodl( obp->ob_ptr );
	obp->ob_ptr += 4;
	return l;
}

/* ogets - Returns a ptr to the next string in the specified object buffer.  */
char* ogets(oblock_t* obp) {

	char	*s;

	s = obp->ob_ptr;
	while( ogetb( obp ));
	return s;
}
/* olodl - Returns the specified long word from the object buffer.  */
long olodl(char* p) {

	return (long)olodw(p+2) << 16 | (long)olodw(p);
}


/* olodw - Returns the specified word from the object buffer.  */
ushort olodw(char* p) {

	return olodb(p+1) << 8 | olodb(p);
}

/* oputb - Puts the specified byte at the end of the object buffer.  */
void oputb(uns b) {

	ostob( b, objblk.ob_top++ );
}

/* oputl - Puts the specified long word at the end of the object buffer.  */
void oputl(long l) {

	char	*p;

	ostob( l, (p = objblk.ob_top) );
	ostob( (l>>8), ++p );
	ostob( (l>>16), ++p );
	ostob( (l>>24), ++p );
	objblk.ob_top = ++p;
}

void aword(ushort w, FILE* file) {/* puts word in specified file */

	putc( w, file );
	w >>= 8;
	putc( w, file );
}

void along(long l, FILE* file) { /* puts long in specified file */

	aword( (int)l, file );
	l >>= 16;
	aword( (int)l, file );
}



/* oputs - Puts the specified string at the end of the object buffer.  */
void oputs(char* s) {

	char	*r;
	r = objblk.ob_top;
	while( *r++ = *s++ );
	objblk.ob_top = r;
}
/* ostol - Stores the specified long word at the specified place in objbuf  */
void ostol(long l, char* p) {

	ostob( (uns) l, p );
	ostob( (uns)( l >> 8 ), p+1 );
	ostob( (uns)( l >> 16 ), p+2 );
	ostob( (uns)( l >> 24 ), p+3 );
}


/* ostow - Stores the specified word at the specified place in objbuf.  */
void ostow(uns w, char* p) {

	ostob( w, p );
	ostob( w >> 8, p+1 );
}
