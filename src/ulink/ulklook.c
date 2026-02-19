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
*			ulk.look.c - symbol table routines		*
*									*
************************************************************************/


static char rcsid[]=
"@(#)$Header: ulklook.c,v 4.5 92/07/17 15:04:08 rmm Rel $ ulink symbol table routines";

#include "ulink.h"

#include <string.h>


/*		symbol table lookup			*/

static sytab_t	**inspt;

/*
 * sylook - Returns a pointer to the symbol table entry for the
 * specified symbol.  Creates a new entry in the symbol table if
 * necessary.
 */

sytab_t *
sylook( s ) char *s;{


	sytab_t	*syp;

	if( syp = sypeek( s )) return( syp ); /* found */

	/* Add a new entry to the table.  */

#ifdef STATS
	syp = (sytab_t *) palloc( sizeof(sytab_t) + strlen(s) - 1, SYMUSE);
#else
	syp = (sytab_t *) palloc( sizeof(sytab_t) + strlen(s) - 1);
#endif
	strcpy( syp->sy_str, s );
	syp->sy_ovl = syp->sy_typ = syp->sy_atr = 0;
	syp->sy_val = 0;
	syp->sy_ord = 0;
	syp->sy_rel = 0;
	syp->sy_lnk = *inspt;
#ifdef xref_t
	syp->sy_xref = 0;	/* null the cross reference chain */
#endif
	return *inspt = syp;
}
/*
 * symerge - Merges the two specified symbol table hash chains, and
 * returns a pointer to the resulting chain.
 */

sytab_t *
symerge( a, b ) sytab_t *a, *b;{


	sytab_t	**re;		/* ptr to last link field in chain */
	sytab_t	*r;			/* resulting chain */

	re = &r;
	if( numorder )
	    while( a && b ){
		if( a->sy_val < b->sy_val ){ /* a is smallest */
			*re = a;
			re = &a->sy_lnk;
			a = a->sy_lnk;
		} else {				/* b is smallest */
			*re = b;
			re = &b->sy_lnk;
			b = b->sy_lnk;
		}
	    }
	else
	    while( a && b ){
		if( strcmp( a->sy_str, b->sy_str )< 0 ){ /* a is smallest */
			*re = a;
			re = &a->sy_lnk;
			a = a->sy_lnk;
		} else {				/* b is smallest */
			*re = b;
			re = &b->sy_lnk;
			b = b->sy_lnk;
		}
	    }
	*re = a ? a: b;
	return r;
}
/*
 * sypeek - Peeks into the symbol table to see if the specified symbol is
 * there.  Returns a pointer to the symbol's entry if it is in the table.
 * Otherwise, sets up inspt to point to the spot where the new entry
 * should be linked in, and returns 0.
 */

sytab_t *
sypeek( s ) char *s;{


	sytab_t	*p,
			*q;
	uns		h;
	int		cmp;
	char	*r;

	h = 0;
	r = s;
	while( *r ) h = (h << 1) + *r++;
	h = ((h*40143) >> 6) & ((1 << HSHLOG) - 1);
	p = 0;
	q = syhtab[h];
	while( q && (cmp = strcmp(s,q->sy_str))> 0 ){ /* follow hash chain */
		p = q;
		q = q->sy_lnk;
	}
	if( q && cmp == 0 ) return q;				/* found */
	inspt = p ? &p->sy_lnk: &syhtab[h];
	return 0;
}

/*
 * numsort sorts a list by value with an insertion sort
 */

sytab_t *
numsort( s ) sytab_t *s; {
	sytab_t *a,*b,*c;
	if( s == NULL ) return s;
	b = s->sy_lnk;
	a = s;
	s->sy_lnk = 0;
	for( s = b; s; s = b ){
		b = s->sy_lnk;
		if( s->sy_val < a->sy_val ){
			s->sy_lnk = a;
			a = s;
		} else{
			for( c=a;
			     c->sy_lnk && c->sy_lnk->sy_val < s->sy_val;
			     c = c->sy_lnk);
			s->sy_lnk = c->sy_lnk;
			c->sy_lnk = s;
		}
	}
	return a;
}

/*
 * sysort - Sorts the symbol table into a single alphabetical chain whose
 * head is syhtab[0].
 */

void sysort(){

	uns		halfi,
			i,
			j;

	if( numorder )
		for( i=0; i<1<<HSHLOG; i++ )
			syhtab[i] = numsort(syhtab[i]);
	for( i = 1 << HSHLOG; i > 1; i = halfi ){
		halfi = i >> 1;
		for( j = 0; j < halfi; j++ )
			syhtab[j] = symerge( syhtab[j], syhtab[j+halfi]);
	}
}
