/************************************************************************
*									*
*	Copyright (C) 1979,1986 by John D. Polstra & Unidot, Inc.	*
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
* Updated 1/2026 by AESilky to compile on current GCC running on 64-bit *
* Linux. No functional changes are intended.				*
*									*
*************************************************************************/

/************************************************************************
*									*
*		    Unidot Syntax Processor				*
*			   Set Ops					*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: uspset.c,v 1.3 86/10/08 22:47:41 jdp Exp $";

#include "usp.h"

static short msk[] = {
	1,2,4,8,0x10,0x20,0x40,0x80,
	0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000 };

#ifndef CLEAN
#define chk(s,b)	if((b) >= SETSIZE)berr(s,b);
berr(s,b)char *s;{fatal("%s(%d)",s,b);}
#else
#define chk(b)
#endif

/* The following declarations are required by closure */
extern DICTENT *termtop;	/* top of terminals in dictionary */
extern PRODENT *prod;		/* base of productions */


/* clearbit - clears the specified bit in the specified set.  */

clearbit( b, s ) short *s;{


	chk("clearbit",b);
	s[b >> S] &= ~msk[b & M];
}

/* clearset - clear the set pointed to by the argument.  */

clearset( s ) short *s;{


	reg	c;

	c = SETSIZE/16;
	do *s++ = 0; while( --c );
}

/*
   closure - expands a state nucleus ( *s ) to include the entire closed
   state.  returns a pointer to the top of the resulting state.  closure
   expects there to be room to expand the nucleus in place; if it ever
   needs to grow beyond lim, it stops and returns 0.
*/

CGRP *
closure( s, lim ) STATE *s;short *lim;{


	CGRP	*bot;	/* first config-group of state			*/
	CGRP	*cur;	/* config-group currently being expanded	*/
	CGRP	*nxt;	/* next config-group to be expanded		*/
	CGRP	*top;	/* top of the state				*/
	CGRP	*cgp;	/* config-group being added			*/
	SET	sct;	/* context set of immediate successors of cur	*/
	PRODENT	*pp;	/* for walking through productions		*/
	int	px;	/* relative productions pointer			*/

	bot = &s->scg[0];
	top = bot + s->ssize;
	for( cur = bot; cur < top; cur = nxt ){

		/* expand each config-group in turn */

		nxt = cur+1;		/* unless we add to previous cgrp */
		if( prod[cur->cpp].pflags & NT ){

			/* has immediate successors */

			scontext( cur, &sct ); /* calculate succ. contexts */
			px = termtop[prod[cur->cpp].pel & BM].dlink;
			while( px ){

				/* look for immediate successors */

				pp = px+prod;
				px = pp->plink;
				if(( pp->pflags&POS )== 0 ){

					/* left part */

					pp++;   /* successor dotted prod. */
					for( cgp = bot;cgp < top;cgp++ )
						/* search */
						if( cgp->cpp == pp-prod ) break;
					if( cgp == top ){

						/* add new cgrp */

						top++;
						if( top > (CGRP *)lim )
							return 0;
						cgp->cpp = pp-prod;
						clearset( &cgp->cset );
					}
					if( cgp<nxt && !subset(&sct,&cgp->cset))
						nxt = cgp; /* reset */
					orset( &sct, &cgp->cset );/* contexts */
				}
			}
		}
	}
	return top;
}

/*
   diffset - subtracts the first set from the second and puts the result
   into the second set.
*/

diffset( s, d ) short *s,*d;{


	reg	c;

	c = SETSIZE/16;
	do *d++ &= ~*s++; while( --c );
}

/* intersect - returns true iff the two sets intersect.  */

intersect( a, b ) short *a,*b;{


	reg	c;

	c = SETSIZE/16;
	do if( *a++ & *b++ ) return 1; while( --c );
	return 0;
}

/* moveset - moves a set from one place to another.  */

moveset( s, d ) short *s,*d;{


	reg	c;

	c = SETSIZE/16;
	do *d++ = *s++; while( --c );
}

/* nullset - returns true iff the specified set is empty.  */

nullset( s ) short *s;{


	reg	c;

	c = SETSIZE/16;
	do if(*s++ ) return 0; while( --c );
	return 1;
}

/* orset - forms the union of two sets and puts the result into the
   second one.  */

orset( s, d ) short *s,*d;{


	reg	c;

	c = SETSIZE/16;
	do *d++ |= *s++; while( --c );
}

/* setbit - sets the specified bit in the specified set.  */

setbit( b, s )short *s;{

	chk("setbit",b);
	s[b >> S] |= 1 << (b&M);
}

/* subset - returns true iff the first set is a subset of the second.  */

subset( a, b ) short *a,*b;{


	reg	c;

	c = SETSIZE/16;
	do if( *a++ & ~*b++ ) return 0; while( --c );
	return 1;
}

/* testbit - tests the specified bit in the specified set.  returns true
   iff the bit is set.  */

testbit( b, s ) short *s;{


	chk("testbit",b);
	return s[b >> S] & (1 << (b&M));
}
