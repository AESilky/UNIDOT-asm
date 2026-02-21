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
*			     UAS Assembler				*
*									*
*			uas.input.c - input routines			*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasinput.c,v 6.4 88/11/20 13:28:05 rmm Rel $ uas input routines";

#include "uas.h"
#include "funcdefs.h"		/* Forward defines for GCC */

#include <fcntl.h> /* For 'open' */
#include <unistd.h> /* For 'close' */
#include <string.h>

/*
 * fillin - Processes the end of an input stack frame.
 */
void fillin(){

	input_t	*rinfp;

	rinfp = infp;
	switch( rinfp->in_typ ){

case INFILE:	rinfp->in_ptr = rinfp->in_buf;
		rinfp->in_cnt = read( rinfp->in_fd, rinfp->in_buf, BUFSIZ);
		if( rinfp->in_cnt != 0 ) return;
		inclev--;
		close( rinfp->in_fd );
		if( inclev == 0 && rinfp->in_seq == 0 )
			warn("83 Input file was empty");
		break;

case INMAC:	mexlev--;		/* flow through to break	*/
case INARG:	break;

case INRPT:	if( rinfp->in_rpt-- == 0 ) break;
		rinfp->in_cnt = rinfp->in_ptr-rinfp->in_buf;
		rinfp->in_ptr = rinfp->in_buf;
		rinfp->in_seq = rinfp->in_fd;
		return;
	}
	popin();
}
/*
 * include - Pushes the specified file into the input stream.  Returns -1
 * if the file cannot be opened, 0 otherwise.
 */
int include(char* file) {
	int		fd;
	int		i;
	int		per;
	char	*p;
	char		filepath[128];
	extern char	srcsuf[1];

	per = 0;
	if( srcsuf[0] )
		for( p=file; *p; p++ )
			if( *p == '.' ){
				per = 1;
				break;
			}
	strcpy( filepath, file );
	if( (fd = open(file, 0)) < 0 && !per ){
		strcat( filepath, "." );
		strcat( filepath, srcsuf );
		fd = open(filepath,0);
	}
	if( fd < 0 ){
		for( i=0; fd < 0 && i < inclx; i++ ){
			strcpy( filepath, inclpath[i] );
			p = filepath + strlen( filepath );
			if( p[-1] != '/' && p[-1] != '\\' && p[-1] != ':' )
				*p++ = '/';
			strcpy( p, file );
			fd = open( filepath, 0 );
			if( fd < 0 && !per ){
				strcat( filepath, "." );
				strcat( filepath, srcsuf );
				fd = open(filepath,0);
			}
		}
		if( fd < 0 ) return -1;
	}
#ifdef msdos
#ifdef MSC
#include <fcntl.h>
	setmode(fd,O_TEXT);
#else
	setmode(fd,"text");
#endif
#endif
	infp = pushin();
	insp += BUFSIZ;
	infp->in_fname = insp;
	i = strlen(filepath) + 1;
	insp += i;
	iovck();
	strcpy( infp->in_fname, filepath );
	infp->in_typ = INFILE;
	infp->in_fd = fd;
	curlst = infp->in_lst & 0xff;
	if( curlst ) curlst--;
	inclev++;
	return 0;
}

/*
 * iovck - Checks to make sure the input stack pointer is not beyond the
 * end of the input stack area.
 */
void iovck(){

	if( insp > ((char *)instk)+INSIZ )
		fatal( "67 Input/Macro stack overflow" );
}
/*
 * popin - Pops the top frame off the input stack.
 */
void popin(){

	curlst = infp->in_lst & 0xff;		/* restore list control	*/
	insp = (char *)infp;			/* pop stack pointer	*/
	infp = infp->in_ofp;			/* pop frame pointer	*/
}

/*
 * pushc - Pushes the specified character onto the input stack.
 */

void pushc(char c) {

	if( insp > ((char *)instk)+INSIZ-1 ) iovck();
	*insp++ = c;
}
/*
 * pushin - Pushes a new frame onto the input stack.  The buffer area in_buf
 * is not allocated, since it is of variable size.  The value returned is
 * the new frame pointer.  Note that the global frame pointer infp is
 * not automatically adjusted.
 */
input_t * pushin(){
	input_t	*newfp;
	int		i;

	while( (int)insp & (ALIGN-1) ) insp++; /* force integer alignment */
	newfp = (input_t *)insp;
	insp = &newfp->in_buf[0];
	iovck();
	newfp->in_ofp = infp;
	newfp->in_seq = newfp->in_cnt = 0;
	newfp->in_lst = curlst;
	newfp->in_fname = 0;
	return newfp;
}
