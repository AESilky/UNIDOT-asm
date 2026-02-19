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
*			uas.misc.c - miscellaneous routines		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasmisc.c,v 6.23 89/03/17 08:15:02 rmm Rel $ uas misc routines";

#include "uas.h"
#include "funcdefs.h"		/* Forward defines for GCC */

#include <stdarg.h>		/* For variable arguments */
#include <stdlib.h>		/* For aligned_alloc */
#define _exit exit
#include <string.h>
#include <unistd.h> 		/* For 'close' and `sbrk` */

void lineover();
int oktobreak(int n);	/* is it ok to break a line after this character */
void prline(FILE* f);

char	*immms1, *immms2;
char	*lstfmt = "%-5s %-11s %-5s ";
char	*errfmt = "====> line %-4u         ";
/*  locationfield objectcode srclinenum			*/
/*  nnnnn nnnnnnnnnnn sssss				*/
/*  0123456789012345678901234		col numbers	*/
/*          |       |       |		tab positions	*/


/*
 * quit - Closes files, then exits with the specified status.  This
 * replaces the normal exit() in libc.a, with its stdio stuff.
 */

void quit( status ) int status;{


#ifndef BIGMEM
	vclose();
#endif
	fclose( stdout );
	fclose( ERRFIL );
	if(LIST)fclose( LIST );
	if(OBJECT)fclose( OBJECT );

#ifdef PROFILE
	monitor( 0 );
#endif // PROFILE

	_exit( status );
}
/*
 * getline - Reads the next line of source, and puts it into the global
 * array sline and into the listing buffer lline.
 */

/*
 * The reason for expanding things out into different stack frames is
 * to simplify the case of breaking lines - much ad hocism here
 */


void ugetline(){

	input_t	*rinfp;
	char	*slp;
	int		ch;
	char	*ap;
	char	*apx;
	char	**avp;
	static char	incfmt[8];

	tokpt = tokpt2 = scanpt = slp = sline;
top:	if( (rinfp = infp) == 0 ){		/* end of file */
		sline[0] = -1;
		sline[1] = '\0';
		return;
	}
	switch( rinfp->in_typ ){

case INMAC:		/* in the middle of expansion of a macro	*/

expmac:		ap = apx = 0;
		llseqval = mexlev;
		llseqfmt = "...%2d";
		for(;;){
			if( ap >= apx ){
				ap = rfetch( (VMADR)rinfp->in_ptr );
				apx = rlimit( (VMADR)rinfp->in_ptr );
			}
			ch = *ap++;
			rinfp->in_ptr++;
			if( ch == 0 )  goto filltop;
			if( ch & 0x80 ){
				avp = (char **)rinfp->in_buf;
				ap = avp[ch & 0x7f];
				if( *ap == 0 ){	/* actual is nil */

					/* KLUDGE: if we are at the
					   start of a line and the next
					   character is a colon, skip it */

					if( slp == sline )
					   while( *rfetch(rinfp->in_ptr)
						== ':' ) rinfp->in_ptr++;
					ap = apx = 0;
					continue;
				}

				/* following gets most cases */

				while( slp < slbr && *ap )
					*slp++ = *ap++;

				/* if we are at end of arg, then done */

				if( *ap == 0 ){
					ap = apx = 0;
					continue;
				}

				/* if there are fewer than 10 chars left */

				if( strlen(ap) < 10 ){
					while( *ap ) *slp++ = *ap++;
					ap = apx = 0;
					continue;
				}

				/* alas, push for break */

				rinfp = infp = pushin();
				rinfp->in_typ = INARG;
				rinfp->in_ptr = ap;
				goto exparg;
			}
			if( ch == '\n' ) goto endl;	/* we have a line */
			*slp++ = ch;
			if( slp > slbr ){
				if( oktobreak(ch) ){
					*slp++ = escchr;
					goto endl;
				}
				if( slp >= &sline[SLINSIZ-2] ) lineover();
			}
		}

case INARG:		/* in middle of macro argument		*/

exparg:
		ap = rinfp->in_ptr;
		while( *slp = ch = *ap++ )
			if( ++slp > slbr ){
				if( oktobreak(ch) ){
					*slp++ = escchr;
					rinfp->in_ptr = ap;
					goto endl;
				}
				if( slp >= &sline[SLINSIZ-2] ) lineover();
			}
		insp = (char *)rinfp;		/* pop stack pointer	*/
		rinfp = infp = infp->in_ofp;	/* pop frame pointer	*/
		goto expmac;

default:					/* file or repeat	*/
		for(;;){
			if( --rinfp->in_cnt < 0 ){ /* end of input frame */
filltop:			fillin();
				goto top;
			}
			ch = *rinfp->in_ptr++;
			if( ch < ' ' ){
				if( ch == '\n' ) break;	/* we have a line */
				if( ch == '\f' ){
					linect = 0;	/* page eject */
					continue;
				}
				if( ch != '\t' ) continue; /* forget trash */
			}
			*slp++ = ch;
			if( slp >= &sline[SLINSIZ-2] )lineover();
		}
		curline = llseqval = ++rinfp->in_seq;
		llseqfmt = "%5u";
		if( inclev >= 0 ){
			llseqfmt = "+%4u";
			if( llseqval < 0 || llseqval > 9999 )
				llseqval = 9999;
		}
		if( rinfp->in_typ == INRPT ) llseqfmt = "R%4u";	/* in Repeat */
	}
endl:	*slp++ = '\n';
	*slp = '\0';
if(debug>5) printf( "%d	%s",curline,sline);
	if( pass2 ){
		*--slp = '\0';
		strcpy( llsrc, sline );
		*slp = '\n';

		/* Decide now whether listing of this line is enabled.  */

		llfull = curlst && ( condlev <= truelev || condlst );
		if( xsline )
			curxpl = ((input_t *)instk)->in_seq;
		else {
			curxpl = (pagect << 6) | (llpp-linect+1) ;
			if( linect == 0 ) /* anticipate form feed */
				curxpl += 99-llpp;
		}
	}
}


int oktobreak(n)int n;{	/* is it ok to break a line after this character */

	if( n == ',' || n == '(' || n == ')' ||
	    n == '{' || n == '}' || n == ' ' ) return 1;
	return 0;
}

void lineover(){
	fatal("34 Input line overflow");
}
/*
 * palloc - Allocates a block of physical memory of the specified size,
 * and returns a pointer to the block.  Minimum allocation: 4 bytes
 */

/*
 * xsbrk - is a surrogate for sbrk-brk in that it has a big stack allocation
 * and then allocates space so that there will always been a lot of
 * headroom for interrupts and the like
 */

char *
xsbrk( size ) uns size; {
//	extern char	*sbrk();
	char autoalloc[1000];		/* move stack down 1000 bytes */
	autoalloc[0] = size;		/* in case of optimization */
	return sbrk(size);
}

#ifdef USEVM
char *
palloc( size ) uns size;{


	static	char	*oldtop = 0;
	char	*pt;
	unsigned int	n;

	if( size & (ALIGN-1) ) size = (size | (ALIGN-1) ) + 1;
	oldtop = phytop;
	phytop += size;
	if( phytop > phylim ){
		pt = sbrk(0);
		if( pt != phylim ){	/* avoid non-contig problem */
			oldtop = phylim = pt;
			phytop = phylim + size;
		}
		while( phytop > phylim ){
#ifdef msdos
			static once = 32768-512;
			n = once;
			once = 0;
			for( ; n >= 8192; n -= 512 )
#else
			static int once = 16384;
			for( n = 16384 ; n >= 512; n -= 512 )
#endif
				if( xsbrk( n ) != (char *)-1 ) break;
			if( n < 512 ){
				if( !vmrq ) fatal( "71 Out of memory" );
				return (char *)-1;
			}
#ifndef msdos
			once = n;
#endif
			phylim += n;
		}
	}
#ifdef msdos
	for( pt=oldtop; pt<phytop; pt++ ) *pt = 0;
#endif
	return oldtop;
}
#else
char* palloc(uns size) {
	return ((char*)aligned_alloc(VMALIGN, size));
}
#endif // USEVM

/*
 * pgcheck - Checks to see if a new listing page is needed, and starts one
 * if necessary.  Then updates the line counter in anticipation of a line
 * of output.
 */

void pgcheck(){

	int	i;

	if( linect <= 0 ){ /* time for a new page */
		fprintf( LIST,
			"\f\n\n%-8s%-*s%s%-*s                Page%4d\n\n",
			prname,
			rmarg-32,
			titl1,
			datstr,
			rmarg-32,
			titl2,
			++pagect );
		linect = llpp;
		filchk( LIST, "listing" );
	}
	linect--;
}

/*
 * putline - Outputs the next line of the assembly listing, provided
 * it is pass 2 and there is something to output.
 */

void putline(){

	int	perr;
	static char	lstnonnull;
	if( pass2 ){
		perr = 0;
		if( llerx || immms1 ) perr = 1;
		if( perr ){
			if( llsrc[0] == 0 ){
				llsrc[0] = lstnonnull;
				prline( ERRFIL ); /* put out error list line */
				llsrc[0] = NULLCA;
			} else
				prline( ERRFIL ); /* put out error list line */
		}
		if( lflag && (llfull || perr)) /* put out assembly listing */
			prline( LIST );
		immms1 = immms2 = 0;
		if( errlim && errct >= errlim )
			fatal("63 Error count exceeds limit of %d",errlim);
	}
	if( llsrc[0] ) lstnonnull = llsrc[0];	/* error reporting purposes */
	*llloc = *llobj = *llseq = llsrc[0] = '\0';
	lllocsiz = 0;
	llerx = 0;
	llobt = llobj;
	aduspc2 = 0;
}


static char ernbuf[6];

char *
ernget(s) char *s;{
	char *p;
	p = ernbuf;
	while( *s == ' ' ) s++;
	while( *s >= '0' && *s <= '9' ) *p++ = *s++;
	*p = 0;
	if( errnum == 0 ) ernbuf[0] = NULLCA;
	while( *s == ' ' ) s++;
	return s;
}

void prline(f)FILE *f;{

	int		i;
	int		j;
	char	*p;
	int		k;
	input_t		*xinfp;

	if( f == LIST ) pgcheck();
	sprintf(llseq,llseqfmt,llseqval);
	if( lllocsiz ) hexit(llloc,lllocsiz,lllocval);
	if( !xsline && f == LIST ) fprintf(f,"%2d ",llpp-linect);

	fprintf(f,lstfmt,llloc,llobj,llseq);

	/* now put out the line, expanding tabs, and truncating to the
	   desired page width	*/

	for( j=0, p = llsrc; j<pgwd && (i = *p); p++,j++ ){
		if( i == '\t' ){
			i = j | 7;
			if( pgwd < i ) i = pgwd;
			while( j < i ) putc( ' ', f ), j++;
			i = ' ';
		}
		putc( i, f );
	}
	putc( '\n', f );
	if( llerx || immms1 ){
		/* first put out the marks	*/
		if( llerx ){
			if( f == LIST ) pgcheck();
			if( !xsline && f == LIST )
				fprintf(f,"%2d ",llpp-linect);
			fprintf(f,errfmt, ((input_t *)instk)->in_seq);
			k = pgwd-24;
			for( j=i=0; i<llerx; i++ ){
				while( j < llerf[i].er_col && j < k )
					putc( ' ', f ), j++;
				if( j == llerf[i].er_col ) putc( '^', f ), j++;
			}
			putc( '\n', f );
		}
		for( i=0; i<llerx; i++ ){
			if( f == LIST ) pgcheck();
			if( !xsline && f == LIST )fprintf(f,"%2d ",llpp-linect);
			fprintf(f,errfmt,((input_t *)instk)->in_seq);
			p = ernget( llerf[i].er_msg );
			fprintf(f, llerf[i].er_flg == ER_WRN ?
					"Warning " : "Error   ");
			if( ernbuf[0] ) fprintf(f,"(%s): ",ernbuf );
			fprintf(f, p, llerf[i].er_par[0], llerf[i].er_par[1]);
			putc( '\n', f );
			xinfp = infp;
			for(;;){
				while( xinfp->in_typ != INFILE &&
				       xinfp->in_ofp ) xinfp = xinfp->in_ofp;
				if( xinfp == (input_t *)instk ) break;
				if( f == LIST ) pgcheck();
				if( !xsline && f == LIST )
					fprintf(f,"%2d ",llpp-linect);
				fprintf(f,errfmt,xinfp->in_seq);
				fprintf(f,"In input file %s\n",xinfp->in_fname);
				xinfp = xinfp->in_ofp;
			}
		}
		if( immms1 ){
			if( f == LIST ) pgcheck();
			if( !xsline && f == LIST )fprintf(f,"%2d ",llpp-linect);
			fprintf(f,errfmt,((input_t *)instk)->in_seq);
			fprintf(f,"%s%s\n",immms1,immms2);
		}
	}
}
/*
 * symcmp - Compares two symbols, and returns a number which is:
 *
 *	> 0, if a > b,
 *	== 0, if a == b,
 *	< 0, if a < b.
 */
int symcmp( a, b ) char *a, *b;{


	int 	i;

	i = SYMSIZ;
	while( --i >= 0 && *a ==*b++ ) if(*a++ == '\0' ) return 0;
	return i < 0 ? 0: *a - *--b;
}

/*
 * symcpy - Copies one symbol from source to destination.
 */

void symcpy( d, s ) char *d, *s;{


	char	i;

	i = SYMSIZ;
	do {
		if( ( *d++ = *s++ ) == '\0' ) return;
	} while( --i > 0 );
}

/*
 * memcpy - Like str(n)cpy but does not stop at a null
 */
//void memcpy( d, s, n ) char *d, *s; int n; {
//	while( --n >= 0 ) *d++ = *s++;
//}


void lcalign( n ) int n; {

	int	i;

	if( pendbits ) nopend();		/* flush */
	i = curloc % n;
	if( i ) curloc += n-i;
}


void range( l, m, n ) long l; int m,n; {

	if( l < m || l > n ) error("13 Value not in range %d-%d",m,n);
}

void hexit( p, n, v ) char *p; int n; long v; {

	/* in the low nibble of n is the field width, in the high nibble is
	   the number of digits to print */

	extern char *hextab;
	int	i;

	i = n >> 4;
	n &= 0xf;
	p[n] = NULLCA;
	while( --i >= 0 ){
		p[--n] = hextab[ v & 0xf ];
		v >>= 4;
	}
	while( --n >= 0 ) p[n] = ' ';
}

/*
 * error - Saves a string for output to the listing - Use either
 * err or error, but not both
 */

void error(char* s, ...) {

	errframe_t		*ep;
	char	*p;
	int		col;

	if( !pass2 ) return;
	if( llerx >= LLERX ) llerx--;
	errct++;
	va_list argptr;
	va_start(argptr, s);
	ep = &llerf[llerx++];
	ep->er_msg = s;
	ep->er_par[0] = va_arg(argptr, int);
	ep->er_par[1] = va_arg(argptr, int);
	for( p = sline, col = 0; p < tokpt; p++, col++ )
		if( *p == '\t' ) col |= 7;
	ep->er_col = col;
	ep->er_flg = 0;
	va_end(argptr);
}

/*
 * warn - Puts the specified warning flag into the listing.
 */

void warn(char* s, ...) {

	if( !pass2 || !verbose ) return;
	error( s );
	llerf[llerx-1].er_flg = ER_WRN;
	errct--;
	warnct++;
}

void fatal(char* s, ...) {

	char	*p;
	int		i;

	va_list argptr;
	va_start(argptr, s);

	s = ernget( s );
	if( ((input_t *)instk)->in_seq ){
		fprintf(ERRFIL,"line %d:\t",((input_t *)instk)->in_seq);
		if( LIST )
			fprintf(LIST,"line %3d:\t\t",((input_t *)instk)->in_seq);
		p = sline;
		while( i = *p++ ){
			fputc( i, ERRFIL );
			if( LIST ) fputc( i, LIST );
		}
		if( p[-1] != '\n' ){
			fputc( '\n', ERRFIL );
			if( LIST ) fputc( '\n', LIST );
		}
	}
	fprintf(ERRFIL,"======>\t\tFATAL   ");
	if( ernbuf[0] ) fprintf(ERRFIL,"(%s): ",ernbuf );
	vfprintf(ERRFIL, s, argptr);
	fprintf(ERRFIL,"\n");
	if( LIST ){
		fprintf(LIST,"======>\t\tFATAL   ");
		if( ernbuf[0] ) fprintf(LIST,"(%s): ",ernbuf );
		vfprintf(LIST, s, argptr);
		fprintf(LIST,"\n");
	}
	va_end(argptr);

#ifdef msdos
	quit(FATEXIT);
#else
	quit(BADEXIT);
#endif
}

void immmsg( s1, s2 ) char *s1; char *s2; {	/* immediate message */

	if( !pass2 ) return;
	immms1 = s1;
	immms2 = s2;
}

void filchk( f, s ) FILE *f; char *s; {
	if( ferror(f) ) fatal("93write error on %s file",s);
}
