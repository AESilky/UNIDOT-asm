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
*			uas.xref.c - cross reference module		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasxref.c,v 6.8 88/02/22 13:52:32 rmm Rel $ uas cross reference";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif

/* putxref - Prints out the cross reference listing.  */

putxref(){

	reg SYTAB	*syp;
	reg XREF	*xrp;
	reg VMADR	sym,
			xr,
			xrtail;
	reg uns		llcol;
	char		undef;
	int		sylen;
	char		*keystr;

	strcpy( titl2, "Cross Reference Listing" );
	linect = 0;
	sysort();
	for( sym = syhtab[0]; sym != 0; sym = syp->sy_lnk ){
		syp = (SYTAB *) rfetch( sym );
		xrtail = syp->sy_xlk;
		undef = syp->sy_typ == STUND || syp->sy_rel == URBUND;
		if( syp->sy_typ == STKEY || syp->sy_typ == STSEC ||
		    undef && xrtail == 0 ) continue;

		pgcheck();
		if( (sylen = strlen(syp->sy_str)) > 32 ) syp->sy_str[32] = 0;
		fprintf( LIST, "%s", syp->sy_str );
		while( ++sylen <= 32 ) fputc( ' ', LIST );
		fputc( ' ', LIST );
		if( syp->sy_typ == STKEQ ){
			xr = syp->sy_val;
			keystr = ((SYTAB *)rfetch(xr))->sy_str;
			sylen = strlen(keystr);
			putblks( 6-sylen );
			fprintf( LIST, "<%s>     ",keystr);
		} else
		if( undef ){
			putblks( 9 );
			fprintf( LIST, "UND " );
		} else {
			fprintf( LIST, "%8lx", (long) syp->sy_val );
			fputc( ' ', LIST );
			if( syp->sy_atr & SAMUD ) fprintf( LIST, "MUL" );
			else
			if( syp->sy_rel == URBABS ) fprintf( LIST, "ABS" );
			else
			if( syp->sy_rel >= URBEXT ) fprintf( LIST, "EXT" );
			else fprintf( LIST, "%3d", syp->sy_rel );
			fputc( ' ', LIST );
		}
		if( xrtail != 0 ){ /* we have some xref entries */
			xrp = (XREF *) rfetch( xrtail );
			llcol = 46;
			do {
				xr = xrp->xr_lnk;
				xrp = (XREF *) rfetch( xr );
				if( llcol > rmarg-8 ){ /* start new line */
					fputc( '\n', LIST );
					pgcheck();
					putblks( 46 );
					llcol = 46;
				}
				if( xsline )
					fprintf( LIST, " %6d%c",
						xrp->xr_pl&0x7fff,
						xrp->xr_pl&XRDEF ? '*': ' ' );
				else
					fprintf( LIST, " %3d-%2d%c",
						xrp->xr_pl>>6&0777,
						xrp->xr_pl&077,
						xrp->xr_pl&XRDEF ? '*': ' ' );
				llcol += 8;
			} while( xr != xrtail );
		}
		syp = (SYTAB *) rfetch( sym );
		fputc( '\n', LIST );
	}
}

putblks(n){
	while( --n >= 0 ) fputc( ' ', LIST );
}
/*
 * symerge - Merges the two specified symbol table hash chains, and
 * returns a pointer to the resulting chain.
 */

VMADR
symerge( a, b ) reg VMADR a, b;{


	reg SYTAB	*ap;
	reg VMADR	pa;
	VMADR		r;
	VMADR		t;
#ifdef BIGMEM
	char		*str;
#else
	char		str[SYMSIZ];
#endif

	if( a == 0 ) return b;
	if( b == 0 ) return a;

	/*
	 * Initialize so that the first element of chain a is less than the
	 * first element of chain b.
	 */

#ifdef BIGMEM
	str = ((SYTAB *)rfetch(b))->sy_str;
#else
	symcpy( str, ((SYTAB *)rfetch(b))->sy_str );
#endif
	if( symcmp(((SYTAB *)rfetch(a))->sy_str,str) > 0 ){

		/* if a > b, exchange a and b */

		t = a;
		a = b;
		b = t;
	}

	r = a;		/* initialize result with smallest element */

	/*
	 * Attach successive smallest portions of the chains onto the result
	 * until there is nothing left.
	 */

	do {
		/*
		 * Walk along chain a until its end is reached or it is
		 * no longer the smaller of the two.
		 */

#ifdef BIGMEM
		str = ((SYTAB *)rfetch(b))->sy_str;
#else
		symcpy( str, ((SYTAB *)rfetch(b))->sy_str );
#endif
		ap=(SYTAB *)rfetch(a);
		do pa = a;
		while( (a = ap->sy_lnk) != 0 &&
			symcmp((ap=(SYTAB *)rfetch(a))->sy_str,str) <= 0 );

		/*
		 * Link b to the end of the result, then exchange the chains
		 * so that a again points to the smaller of the two.
		 */

		((SYTAB *)wfetch(pa))->sy_lnk = b;
		t = a;
		a = b;
		b = t;
	} while( b != 0 );
	return r;
}

/*
 * lstsort - Sort a single symbol list 
 */

VMADR
lstsort( l ) reg VMADR l; {

	reg SYTAB	*cp,
			*bp;
	reg VMADR	a,b,c,d;
#ifdef BIGMEM
	char		*str;
#else
	char		str[SYMSIZ+1];
#endif

	a = 0;
	while( l ){
#ifdef BIGMEM
		str = ((SYTAB *)rfetch(l))->sy_str;
#else
		symcpy(str,((SYTAB *)rfetch(l))->sy_str);
#endif
		c = a;
		d = 0;
		while( c && symcmp((cp=(SYTAB *)rfetch(c))->sy_str,str) < 0 ){
			d = c;
			c = cp->sy_lnk;
		}
		bp = (SYTAB *)wfetch(l);
		b = bp->sy_lnk;
		bp->sy_lnk = c;
		if( d )		((SYTAB *)wfetch(d))->sy_lnk = l;
			else	a = l;
		l = b;
	}
	return a;
}


/*
 * sysort - Sorts the symbol table into a single alphabetical chain whose
 * head is syhtab[0].
 */

sysort(){

	reg uns		halfi,
			i,
			j;

	for( i = 0; i < (1 << SHSHLOG); i++ ) syhtab[i] = lstsort(syhtab[i]);
	for( i = 1 << SHSHLOG; i > 1; i = halfi ){
		halfi = i >> 1;
		for( j = 0; j < halfi; j++ ){
			syhtab[j] = symerge( syhtab[j], syhtab[j+halfi]);
		}
	}
}
/*
 * xref - Adds a cross reference entry of the specified type for the
 * specified symbol.
 */

xref( sym, type ) VMADR sym; int type;{


	reg SYTAB	*syp;
	reg XREF	*nxp;
	reg VMADR	nxr;
	reg VMADR	oxr;
	int		pl;

	if( !(xflag && pass2) ) return;
	pl = (curxpl & 0x7fff)|type;

	/*
	 * CAUTION -- This code requires an lru virtual memory buffer pool
	 * of at least 3 blocks.
	 */

	syp = (SYTAB *) wfetch( sym );
	oxr = syp->sy_xlk;
	valign();			/* align virtual mem */
	if( oxr == 0 ){			/* first xref entry for this symbol */

		nxr = VALN(sizeof(XREF));
		nxp = (XREF *)wfetch(nxr);
		nxp->xr_lnk = nxr;

	} else {	/* add to existing circular list of xref entries */

		nxp = (XREF *) wfetch( oxr );
		if( nxp->xr_pl == pl ) return;		/* no duplicate xrefs */
		oxr = nxp->xr_lnk;
		nxp->xr_lnk = nxr = VALN(sizeof(XREF));
		nxp = (XREF *)wfetch(nxr);
		nxp->xr_lnk = oxr;
	}
	nxp->xr_pl = pl;
#ifndef BIGMEM
	syp = (SYTAB *) wfetch( sym );			/* paranoia	*/
#endif
	syp->sy_xlk = nxr;
}
