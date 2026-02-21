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
*			uas.look.c - table lookup routines		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uaslook.c,v 6.6 88/03/10 09:31:38 rmm Rel $ uas table lookup routine";

#include "uas.h"
#include "funcdefs.h"		/* Forward defines for GCC */
#include <string.h>

#ifndef NOPD
#define NOPD	/* ES: Define this here to aid code highlighting (is in makefile) */
#endif

#ifndef BIGMEM
#define ASIDE struct aside
ASIDE {			/* symbol lookaside table entry */
	ASIDE	*as_lnk;		/* lru chain link */
	VMADR	as_sym;			/* symbol table entry pointer */
	char	as_str[SYMSIZ];		/* symbol */
};
static ASIDE	aspool[] = {
	{ &aspool[1] },{ &aspool[2] },{ &aspool[3] },
	{ &aspool[4] },{ &aspool[5] },{ &aspool[6] },
	{ &aspool[7] },{ &aspool[8] },{ &aspool[9] },
	{ &aspool[10] },{ &aspool[11] },{ &aspool[12] },
	{ &aspool[13] },{ &aspool[14] },{ &aspool[15] },
	{ &aspool[16] },{ &aspool[17] },{ &aspool[18] },
	{ &aspool[19] },{ &aspool[20] },{ &aspool[21] },
	{ &aspool[22] },{ &aspool[23] },{ &aspool[24] },
	{ &aspool[25] },{ &aspool[26] },{ &aspool[27] },
	{ &aspool[28] },{ &aspool[29] },{ 0 } };
static ASIDE	*ashead = &aspool[0];
#endif
/*
 * oclook - Returns a pointer to the operation table entry for the
 * specified opcode symbol.  Creates a new entry in the operation
 * table if necessary.
 *
 * In the original, both the upper and lower case versions of the opcode
 * were in the table.  In the current schema, only the lower case version
 * is stored.  The lookup proceeds as follows:  First we search for the
 * symbol as supplied.  This will find exact matches for macros, etc that
 * are programmer supplied.  If there is no hit, we convert the symbol
 * to all lower case and look again.  This time we must not only get a
 * match, but it must not be a programmer defined macro.  If this fails
 * we enter the original symbol as supplied.
 */


octab_t* oclook(char* s) {


	octab_t	*q;
	char	*b;
	char	*a;
	int		i;
	uns		h;
	static char	opstr[16] = {NULLCA};

	h = hash(s) & ((1 << OHSHLOG)-1);
	b = s;
	for( q = ochtab[h]; q; q = q->oc_lnk )
		if( symcmp( b, q->oc_str ) == 0 ) return q;

	/* now convert to lower case */

	a = opstr;
	while( (i = *b++) != 0 ){
		if( i >= 'A' && i <= 'Z' ) i += 'a' - 'A';
		*a++ = i;
	}
	*a = 0;
	for( q = ochtab[h]; q; q = q->oc_lnk )
		if( symcmp( opstr, q->oc_str ) == 0 &&
		    (upperonly || q->oc_typ != OTMAC)) return q;

	/* Add a new entry to the table. (Use original string and hash
	   unless the uppercase only flag is set) */

	if( noentry ) return (octab_t *)0;
	if( upperonly ) s = opstr;
	if( pass2 ) fatal("57 No new ops (%s) in pass2",s);
	q = (octab_t *) palloc( sizeof(octab_t) - SYMSIZ + strlen(s) + 1 );
	symcpy( q->oc_str, s );
	q->oc_typ = 0;
	q->oc_lnk = ochtab[h];
	ochtab[h] = q;
	return q;
}

/* following routine returns oc_val (or 0) without making an entry in
   the table to avoid cluttering up the opcode table during long
   (and strange macro definitions).. Since this routine has limited
   uses, it must be called with the string already converted to lower
   case.
*/
int opval(char* s) {


	octab_t	*q;
	int		h;

	h = hash(s) & ((1 << OHSHLOG) - 1);
	for( q = ochtab[h]; q; q = q->oc_lnk )
		if( symcmp( s, q->oc_str ) == 0 )
			return (int)(q->oc_val);
	return 0;
}

#ifdef NOPD
	/* quick routine for inserting pre-built opcodes into chain */

void opcinsert(octab_t* o) {

	uns		h;

	h = hash( o->oc_str ) & ((1 << OHSHLOG) - 1);
	o->oc_lnk = ochtab[h];
	ochtab[h] = o;
}
#endif
/*
 * hash - Given a string, computes a partial hashing function of the string,
 * and returns its value.  Same hash value in upper or lower case.
 */
uns hash(char* s) {


	uns		h;

	h = 0;
	while( *s ) h = (h << 1) + (*s++ & ~040);
	return h*40143;
}

/*
 * sylook - Returns a virtual memory pointer to the symbol table entry
 * for the specified symbol.  Creates a new entry in the symbol table
 * if necessary.
 */


VMADR sylook(char* s) {


#ifndef BIGMEM
	ASIDE	*apt,
			**lpt;
#endif
	sytab_t	*qp,
			*rp;
	VMADR		p;
	VMADR		q;
	VMADR		r;
	uns		h;
	int		cmp;

#ifdef	STATS
	sylct++;
#endif

#ifndef BIGMEM
	/* Check the lookaside table to see if the symbol is in it.  */

	lpt = &ashead;
	apt = ashead;
	while( (cmp = symcmp( apt->as_str, s )) && apt->as_lnk )
		apt = *( lpt = &apt->as_lnk );

	/*
	 * Whether we found a hit or not, move the entry to the front of the
	 * lru chain.
	 */

	if( !noentry ){
		*lpt = apt->as_lnk;
		apt->as_lnk = ashead;
		ashead = apt;
	}
	if( cmp == 0 ){ /* hit */
#ifdef	STATS
		ashct++;
#endif
		return apt->as_sym;
	}

	/*
	 * No luck in the lookaside table, so search the hash chains in
	 * the customary manner.
	 */

	if( !noentry ) symcpy( ashead->as_str, s ); /* update lookaside table */
#endif
	h = hash(s) & ((1 << SHSHLOG) - 1);
	p = 0;
	q = syhtab[h];
	while( q &&
	       (cmp = symcmp( s,(qp =(sytab_t *)rfetch(q))->sy_str)) > 0 ){
		p = q;
		q = qp->sy_lnk;
#ifdef	STATS
		chnct++;
#endif
	}

#ifndef BIGMEM
	if( q && cmp == 0 ){				/* found */
		if( !noentry ) ashead->as_sym = q;	/* update lookaside */
		return q;
	}
#else
	if( q && cmp == 0 ) return q;
#endif

	if( noentry ) return 0;

	/* Add a new entry to the table.  */

#ifdef	STATS
	symct++;
#endif

	/* another place for 4 byte alignment RMM 7/26/85 */

	valign();					/* align vm */
	cmp = sizeof(sytab_t) - SYMSIZ + strlen(s) + 1;	/* entry size	*/
	r = VALN(cmp);					/* get another spot */
	rp = (sytab_t *) wfetch(r);
	symcpy( rp->sy_str, s );
	rp->sy_typ = rp->sy_atr = rp->sy_rel = 0;
	rp->sy_val = 0;
	rp->sy_xlk = (void*)0;
	rp->sy_lnk = q;
	if( p ) (qp=((sytab_t *)wfetch( p )))->sy_lnk = r;
	else syhtab[h] = r;
#ifndef BIGMEM
	return ashead->as_sym = r;
#else
	return r;
#endif
}

VMADR numlab(unsigned n) {		/* find the relevant numeric label */

	numlab_t	*nml;
	VMADR		nnn;
	VMADR		mmm;
	int		nx;

	if( nchd == 0 )			/* no numchain yet */
		nchd = nctl = (numchn_t *)palloc(sizeof(numchn_t));
	nx = n % NMCCNT;
	nnn = nctl->nc_nm[nx];
	while( nnn ){
		nml = (numlab_t *) rfetch( nnn );
		if( nml->nm_lab == n ) return nnn;
		nnn = nml->nm_lnk;
	}

	/* no such label - append one */

	valign();					/* align vm */
	nnn = VALN( sizeof(numlab_t) );
	nml = (numlab_t *)wfetch( nnn );
	nml->nm_lnk = nctl->nc_nm[nx];
	nml->nm_lab = n;
	nctl->nc_nm[nx] = nnn;
	return nnn;
}
