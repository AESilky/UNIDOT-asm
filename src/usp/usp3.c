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
*			   USP 3					*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: usp3.c,v 1.3 86/10/08 22:47:17 jdp Exp $";

#include <stdarg.h> 		/* For va_arg (ES) */
#include <stdio.h>
#include "usp.h"
#include <signal.h>
#include <stdlib.h>		/* For exit (and others) (ES) */
#include <fcntl.h> 		/* For 'open' (ES) */
#include <unistd.h> 		/* For 'close' and `sbrk` (ES) */

DICTENT	*dict;			/* base of dictionary			*/
DICTENT	*termtop;		/* top of terminals			*/
DICTENT	*goaltop;		/* top of goal symbols			*/
DICTENT	*nontermtop;		/* top of nonterminals			*/
DICTENT	*dictop;		/* top of dictionary			*/

char	*string;		/* base of strings			*/
char	*sttop;			/* top of strings			*/
char	*oname;			/* output table name			*/
// char	*sbrk();		/* memory getter			*/
char	*memtop;		/* top of occupied memory		*/
char	*memlim;		/* end of acquired memory		*/
char	obuf[BUFSIZ];		/* output buffer			*/

PRODENT	*prod;			/* base of productions			*/
PRODENT	*prodtop;		/* top of productions			*/

SET	*hs;			/* base of headsets			*/
SET	*hstop;			/* top of headsets			*/

STATE	*stab;			/* base of states			*/
#define ST(a)	((STATE *)(a))	/* cast to a state pointer	*/
#define st(a)	((STATE *)((a) + (short *)stab))	/* stab ptr */
#define Sx(a)	((short *)(a) - (short *)(stab))	/* stab index */
STATE	*cstate;		/* current state being expanded		*/
STATE	*fstate;		/* first state in list			*/
STATE	*lstate;		/* last state in list			*/
STATE	*stop;			/* top of states			*/

int	dfile;
int	pfile;
int	sfile;
int	hfile;

short	clo[CLOSIZE];		/* closure of current state		*/
short	ocol;			/* output column position		*/
short	debug;			/* debug flag				*/
short	Zflag;			/* no execl flag			*/
short	slist;			/* stab listing flag			*/

CGRP	*closure();
CGRP	*statemove();
#ifdef msdos
int	_iomode = 0;
#endif

/*
   Internal function declarations. (ES)
*/
STATE* bfs();
void bsuccs(STATE* s);
void buildhs();
void buildstab();
LIST* bxl(CGRP** xtab, CGRP** xtop);
CGRP** bxs(STATE* xlp, CGRP** xtp, CGRP** xtop);
int compatible(STATE* s, STATE* t);
void fatal(char* s, ...);
void fhs(int nt);
void grow();
void listcg(CGRP* cgp);
void listset(SET* s);
void liststate(STATE* s, CGRP* top);
void listxl(LIST* xlp);
void merge(STATE* a, STATE* b);
int opfile(char* s);
void putch(char ch);
void putst(char* s);
char* readblock(int i);
void readdict();
void readprods();
void rmfiles();
void scontext(CGRP* cgp, SET* csp);
CGRP* statemove(short* a, short* b, short* lim);
void writeblock(int i, char* first, char* limit);
void writehs();
void writestab();
void xcomp(CGRP** a, CGRP** b);



void intr(){ fprintf(stderr,"usp3 interrupt!\n"); rmfiles(); }


int main(int argc, char** argv) {


	char	*flags;	/* pointer to flags */
	char	*fp;	/* pointer into flags for scanning */

	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
	if( argc < 3 ) fatal("argc error");
	oname = argv[2];
	flags = fp = argv[1];
	if( oname[0] == '-' ){
		oname = argv[1];
		flags = fp = argv[2];
	}
	while( *++fp ) switch( *fp ){		/* crack the flags */
case 'd':	debug = 1; continue;
case 's':
case 'v':	slist = 1; continue;
case 'Z':	Zflag = 1; continue;
	}
	setbuf( stdout, obuf );
	fprintf( stderr, "usp3:	%s\n",oname );
	memtop = memlim = sbrk( 0 );
	dfile = opfile( dname );
	pfile = opfile( pname );
	readdict();
	readprods();
	close( dfile );
	close( pfile );
	buildhs();
	buildstab();
	hfile = opfile( hname );
	sfile = opfile( sname );
	writehs();
	writestab();

	/* chain to the next phase.  */

	fflush( stdout );
	close( hfile );
	close( sfile );
#ifdef msdos
	exit(0);
#else
	if( Zflag ) exit( 0 );
	execl( "usp4", "usp4", flags, oname, 0 );
	execl( "/usr/lib/usp4", "usp4", flags, oname, 0 );
	fatal( "cannot reach usp4" );
#endif
}

/*
   bfs - build the first state at the top of memory and return a pointer
   to it.  the config-groups are built sorted on their production 
   pointers, as are all states.
*/

STATE* bfs(){

	CGRP	*cgp;	/* scanning pointer			*/
	STATE	*base;	/* pointer to the state			*/
	DICTENT	*dp;	/* dictionary ptr for scanning goal syms */
	CGRP	*bot;	/* bottom config-group			*/
	CGRP	*top;	/* top of config-groups			*/
	short		px;	/* relative productions pointer		*/

	base = (STATE *) memtop;
	bot = top = &base->scg[0];

	for( dp = termtop; dp < goaltop; dp++ ){
		px = dp->dlink;
		while( px ){
			memtop = (char *)(top+1);
			if( memtop > memlim ) grow();
			clearset( &top->cset );
			setbit( 0, &top->cset );
			for( cgp = top-1; cgp >= bot; cgp-- ){
				if( px+1 >= cgp->cpp ) break;
				(cgp+1)->cpp = cgp->cpp;
			}
			(cgp+1)->cpp = px+1;
			top = (CGRP *)memtop;
			px = prod[px].plink;
		}
	}
	base->ssize = top-bot;
	base->slink = base->xlink = 0;
	if( debug ){
		printf( "initial state %d:\n", Sx(base) );
		liststate( base, &base->scg[base->ssize]);
		printf( "\n" );
	}
	return base;
}

/*
   bsuccs - build all of the successors of the specified state.  check
   each successor for compatibility with existing states, and merge
   if possible.  this is the meat of the whole algorithm.
*/

void bsuccs(STATE* s) {


	CGRP	*cgp;	/* for scanning state			*/
	LIST	*xlp;	/* pointer for walking through xlist	*/
	CGRP		*ctop;	/* top of closure (in clo)		*/
	CGRP		**xtop;	/* top of xtab				*/
	CGRP		**xtp;	/* scan pointer for xtab		*/
	CGRP		**rtop;	/* top of rtab				*/
	LIST		*xlbase; /* pointer to base of transition list	*/
	CGRP		*xtab[XSIZE];	/* transition table		*/
	CGRP		*rtab[RSIZE];	/* reduction table		*/

	extern		xcomp(); /* transition comparison routine	*/

	/* copy the state to clo and close it */

	if( !statemove( (short *)s, clo, clo+CLOSIZE ) ||
	    (ctop = closure( ST(clo), clo+CLOSIZE)) == 0 )
		fatal( "CLOSIZE too small" );

	/* record transitions and reductions in xtab and rtab, resp. */

	xtop = xtab;
	rtop = rtab;
	for( cgp = &((STATE *)clo)->scg[0]; cgp < ctop; cgp++ )
		if( prod[cgp->cpp].pflags & SEM ){	/* reduction */
			if( rtop >= &rtab[RSIZE]) fatal( "RSIZE too small" );
			*rtop++ = cgp;
		} else {				/* transition */
			if( xtop >= &xtab[XSIZE]) fatal( "XSIZE too small" );
			*xtop++ = cgp;
		}

	/* sort xtab by transition symbol and production pointer */

	qsort( xtab, xtop-xtab, sizeof(CGRP *), xcomp );

	/* build a skeleton transition list */

	xlbase = bxl( xtab, xtop );
	s->xlink = Sx(xlbase);

	/* build the successor states and fill in the transition list */

	xtp = xtab;
	for( xlp = xlbase; !(xlp->xflags & XEND); xlp++ )
		xtp = bxs( ST(xlp), xtp, xtop );
}

/*
   buildhs - builds the headsets for all nonterminal symbols.
*/

void buildhs(){

	DICTENT	*dp;
	int		nt;

	hs = (SET *)memtop;
	hstop = &hs[nontermtop - termtop];
	while( hstop > (SET *) memlim ) grow();
	memtop = (char *)hstop;
	for( dp = termtop; dp < nontermtop; dp++ ) /* clear all CM flags */
		dp->dflags &= ~CM;
	for( nt = 0; nt < nontermtop-termtop; nt++ ) /* find headsets */
		fhs( nt );
}

/*
   buildstab - builds the state table data structure.  when it is all done,
   the state table is between stab and stop.
*/

void buildstab() {

	int	csx;	/* relative pointer to current state */

	stab = (STATE *)memtop;
	fstate = lstate = bfs();
	csx = Sx(fstate);
	do bsuccs( cstate = st(csx) ); while( (csx = cstate->slink) != 0 );
	stop = (STATE *)memtop;
	if( slist ){
		printf( "\nstate table list:\n\n" );
		csx = Sx(fstate);
		do{
			printf( "state %d:\n", csx );
			liststate( st(csx),&st(csx)->scg[st(csx)->ssize]);
			listxl( (LIST *)st(st(csx)->xlink) );
			printf( "\n" );
		} while( (csx = st(csx)->slink) != 0 );
	}
}

/*
   bxl - builds a transition list skeleton and returns a pointer to it.
   the skeleton has everything filled in except the xsuc field.
*/

LIST* bxl(CGRP** xtab, CGRP** xtop) {


	LIST	*xlp;
	PRODENT	*pp;
	LIST	*xbase;
	short		f;
	short		s;

	xbase = xlp = (LIST *)memtop;
	while( xtab < xtop ){
		memtop = (char *)(xlp+1);
		if( memtop > memlim ) grow();
		pp = (*xtab)->cpp+prod;
		xlp->xflags = f = pp->pflags&NT;
		xlp->xsym = s = pp->pel&BM;
		xlp->xsuc = 0;
		do{
			if( ++xtab >= xtop ) break;
			pp = (*xtab )->cpp+prod;
		} while( (pp->pel&BM) == s && (pp->pflags&NT) == f );
		xlp = (LIST *)memtop;
	}
	memtop = (char *)(xlp+1);
	if( memtop > memlim ) grow();
	xlp->xflags = XEND;
	return xbase;
}

/*
   bxs - build transition successor state.  is called with a pointer
   to an xlist entry, a pointer into xtab, and a pointer to the end
   of xtab.  builds the successor state, checks for compatibility and
   merges it with a previous state if possible, fills in the xlist
   entry, and returns a new pointer into xtab where the next transition
   successor starts.
*/

CGRP** bxs(STATE* xlp, CGRP** xtp, CGRP** xtop) {


	CGRP	*bot;		/* bottom cgrp of new state */
	CGRP	*top;		/* top of new state */
	STATE	*base;		/* base of new state being built */
	STATE	*nstate;	/* ptr to new state after possible merging */
	short	f;		/* flags */
	short	s;		/* transition symbol */
	short	px;		/* index in productions */
	short	sx;		/* index in stab */

	base = (STATE *)memtop;
	bot = top = &base->scg[0];
	px = (*xtp)->cpp;
	f = prod[px].pflags & NT;
	s = prod[px].pel & BM;
	do {			/* put config-groups in new state */
		memtop = (char *)(top+1);
		if( memtop > memlim ) grow();
		top->cpp = px+1;
		moveset( &(*xtp)->cset, &top->cset );
		top = (CGRP *)memtop;
		if( ++xtp >= xtop ) break;
		px = (*xtp )->cpp;
	} while( (prod[px].pel & BM) == s && (prod[px].pflags & NT) == f );
	base->ssize = top-bot;
	base->slink = base->xlink = 0;
	nstate = base;
	sx = Sx(fstate);
	do {			/* search for compatible state */
		if( compatible(base, st(sx)) ){
			nstate = st(sx);
			break;
		}
	} while( (sx = st(sx)->slink) != 0 );
	if( nstate != base ){	/* found a compatible state */
		if( debug ){
			printf( "merging into state %d:\n", Sx(nstate) );
			liststate( base, &base->scg[base->ssize]);
			printf( "\n" );
		}
		merge( base, nstate );
		memtop = (char *)base;
	} else {		/* link new state into the chain */
		if( debug ){
			printf( "creating new state %d:\n", Sx(base) );
			liststate( base, &base->scg[base->ssize]);
			printf( "\n" );
		}
		lstate->slink = Sx(base);
		lstate = base;
	}
	((LIST *)xlp)->xsuc = Sx(nstate);
	return xtp;
}

/*
   compatible - returns true iff the two states are weakly compatible.
*/

int compatible(STATE* s, STATE* t) {


	SET	*siset;
	SET	*tiset;
	SET	*sjset;
	SET	*tjset;
	short	i;
	short	j;
	short	size;

	if( (size = s->ssize) != t->ssize ) return 0;
	for( i = 0; i < size; i++ )
		if( s->scg[i].cpp != t->scg[i].cpp ) return 0;
	for( j = 1; j < size; j++ ){
		siset = &s->scg[i].cset;
		tiset = &t->scg[i].cset;
		for( i = 0; i < j; i++ ){
			sjset = &s->scg[j].cset;
			tjset = &s->scg[j].cset;
			if( (intersect( siset, tjset) ||
			    intersect( tiset, sjset)) &&
			      !intersect( siset, sjset) &&
			      !intersect( tiset, tjset))	return 0;
		}
	}
	return 1;
}

  
/*
   fhs - finds the headset for the specified nonterminal symbol.  the
   CM flag is used to record those nonterminals for which the headset
   has already been found, to avoid redundant calculations.
*/

void fhs(int nt) {


	DICTENT	*dp;
	PRODENT	*pp;
	short		px;
	short		el;

	dp = nt+termtop;
	if( !(dp->dflags & CM) ){	
	
		/* must calculate the headset */

		dp->dflags |= CM;		/* don't calculate it again */
		clearset( nt+hs );
		px = dp->dlink;
		while( px ){
			pp = &prod[px];
			px = pp->plink;
			if( (pp->pflags & POS) == 0 ){	/* found LP */
				do{
					pp++;
					el = pp->pel & BM;
					if( pp->pflags & SEM ) break;
					if( pp->pflags & NT ){
						fhs( el );
						orset( &hs[el], &hs[nt] );
					} else {
						setbit( el, &hs[nt] );
						break;
					}
				} while( termtop[el].dflags & MT );
			}
		}
	}
}

void grow(){

	if( sbrk( 2048 ) == (char *)-1 ) fatal( "out of memory" );
	memlim += 2048;
	if( debug ) fprintf( stderr, "growing by 2k bytes\n" );
}

/*
   listcg - lists a config-group, including the dotted production and
   the context set.
*/

void listcg(CGRP* cgp) {

	DICTENT	*dp;
	PRODENT	*pp;
	PRODENT	*dot;

	/* first list the dotted production.  */

	dot = &prod[cgp->cpp];
	pp = dot - (dot->pflags & POS);
	do {
		dp = (pp->pflags & NT) ?
			&termtop[pp->pel & BM] :
			&dict[pp->pel & BM];
		printf( "%s ", &string[dp->dstring] );
		pp++;
		if( (pp->pflags & POS) == 1 ) printf( "::= " );
		if( pp == dot ) printf( ". " );
	} while( !(pp->pflags & SEM) );
	printf( "%d\n", pp->pel & BM );
	listset( &cgp->cset );
}

/*
   listset - lists the members of the specified set.
*/

void listset(SET* s) {


	DICTENT	*dp;
	char	*sp;
	short		t;
	short		first;

	first = 1;
	ocol = 1;
	putch( '{' );
	for( t = 0, dp = dict; dp < termtop; t++, dp++ ){
		if( testbit( t, s ) ){			/* found a member */
			if( !first ) putch( ' ' );
			sp = dp->dstring+string;
			if( ocol + strlen( sp ) > RMARG ) putch( '\n' );
			putst( sp );
			first = 0;
		}
	}
	putst( "}\n" );
}

/*
   liststate - lists all the config-groups of the specified state.  the
   input parameters are pointers to the bottom and top of the state,
   respectively.
*/

void liststate(STATE* s, CGRP* top) {


	CGRP	*cgp;

	for( cgp = &s->scg[0]; cgp < top; cgp++ ) listcg( cgp );
}

/*
   listxl - lists a transition list (xlist).
*/

void listxl(LIST* xlp) {


	DICTENT	*dp;

	while( !(xlp->xflags & XEND) ){
		dp = xlp->xflags & NT ?
			&termtop[xlp->xsym & BM] :
			&dict[xlp->xsym & BM];
		printf( "%s ==> %d\n", &string[dp->dstring], xlp->xsuc );
		xlp++;
	}
}

/*
   merge - merges the first state into the second.  the two states are
   assumed to be compatible.  successors are checked and updated as
   necessary.
*/

void merge(STATE* a, STATE* b) {


	CGRP	*bp;	 /* for walking through b */
	CGRP	*cp;	 /* for walking through c */
	short		*c;	 /* pointer to closure area for new contexts */
	CGRP 		*ctop;	 /* top of c */
	short		*nct;	 /* pointer to successor new contexts */
	short		*suc;	 /* current successor being considered */
	short		*suctop; /* top of suc */
	CGRP		*nctp;	 /* for walking through nct */
	CGRP		*ncttop; /* top of nct */
	short		*sucp;	 /* for walking through suc */
	LIST		*xlp;	 /* for walking through xlist */
	short		propsym; /* special "terminal" for tracing contexts */
	short		hflag;	 /* set iff some context was enlarged by merge*/

	/* first copy the new state nucleus into c, and set c's contexts to the
	   new symbols which are being added to b's contexts.  */

	propsym = termtop - dict; /* first # beyond highest terminal */
	c = (short *)memtop;
	while( (ctop = statemove( (short *)a, c, (short *)memlim )) == 0 )
		grow();
	memtop = (char *)c;
	bp = &b->scg[0];
	cp = &ST(c)->scg[0];
	hflag = 0;
	while( cp < ctop ){
		diffset( &bp->cset, &cp->cset );	/* new symbols */
		orset( &cp->cset, &bp->cset );	/* do the merge into b */
		if( !nullset( &cp->cset ) ){
			setbit( propsym, &cp->cset );
			hflag = 1;
		}
		bp++;
		cp++;
	}
	if( !hflag || b->xlink == 0 ){	/* no change or no successors */
		memtop = (char *)c;		/* give back the memory */
		return;
	}
	if( b == cstate ){		/* must reclose current state */
		if( !statemove( (short *)cstate, clo, clo+CLOSIZE ) ||
		    closure( ST(clo), clo+CLOSIZE ) == 0 )
			fatal( "reclose" );
	}

	/* now form the closure of c in order to determine contexts propogated
	   or generated for successors.  */

	while( (ctop = closure( ST(c), (short *)memlim )) == 0 ) grow();
	memtop = (char *)ctop;

	/* now propagate new contexts to successor states and
	   verify compatibilty.  */

	for( xlp = (LIST *)st(b->xlink); !(xlp->xflags & XEND); xlp++ ){
		if( xlp->xsuc == 0 ){	/* successor not generated yet */
			memtop = (char *)c;	/* return the memory */
			return;
		}
		suc = (short *)st(xlp->xsuc);
		suctop = (short *)&ST(suc)->scg[ST(suc)->ssize];
		if( debug ) printf("checking successor state %d:\n", Sx(suc) );
		for( sucp = (short *)&ST(suc)->scg[0]; sucp < suctop; sucp++ ){
			for( cp = &ST(c)->scg[0]; cp < ctop; cp++ )
				if( ((CGRP *)sucp)->cpp == cp->cpp+1 &&
				 testbit( propsym, &cp->cset )) break;
			if( cp < ctop ) break;
		}
		if( sucp == suctop ) continue; /* this successor unchanged */

		/* we have found a successor state which must be updated.  */

		nct = (short *)memtop;
		while( (ncttop = statemove(suc, nct, (short *)memlim)) == 0 )
			grow();
		memtop = (char *)ncttop;
		for( nctp = &ST(nct)->scg[0]; nctp < ncttop; nctp++ ){
			for( cp = &ST(c)->scg[0]; cp < ctop; cp++ )
				if( nctp->cpp == cp->cpp+1 ) break;
			if( cp < ctop ){
				moveset( &cp->cset, &nctp->cset );
				clearbit( propsym, &nctp->cset );
			} else
				clearset( &nctp->cset );
		}
		if( debug ){
			printf( "adding nct to successor:\n" );
			liststate( ST(nct), &ST(nct)->scg[ST(nct)->ssize]);
			printf( "\n" );
		}
		if( compatible( ST(nct), ST(suc) )){
			merge( ST(nct), ST(suc) );
		} else {
			printf( "*** successors incompatible:\n" );
			printf( "suc:\n" );
			liststate( ST(suc), (CGRP *)suctop );
			printf( "nct:\n" );
			liststate( ST(nct), (CGRP *)ncttop );
		}
		memtop = (char *)nct;		/* return the memory */
	}
	memtop = (char *)c;			/* return the memory */
}

/*
   putch - output the specified character and maintain the output column
   position in ocol.
*/

void putch(char ch) {


	if( ch == '\n' ) ocol = 0;
	putchar( ch );
	ocol++;
}

/* putst - output the specified string.  */

void putst(char* s) {


	while( *s ) putch( *s++ );
}

char* readblock(int i) {


	ushort	length;
	short 	al;
	char	*oldtop;

	al = read( i, (char *)&length, sizeof(short) );
	if( al != sizeof(short) )
		fatal( "blocklength read error, al = %d", al );
	while( memtop+length > memlim ) grow();
	al = read( i, memtop, (int)length );
	if( al != length ) fatal( "format error, al = %d", al );
	oldtop = memtop;
	if( length & 1 ) length++;		/* short align */
	memtop += length;
	return oldtop;
}

void readdict() {

	lseek( dfile, 0L, 0 );
	dict = (DICTENT *)readblock( dfile );		/* terminals	*/
	termtop = (DICTENT *)readblock( dfile );	/* goal symbols */
	goaltop = (DICTENT *)readblock( dfile );	/* nonterminals */
	nontermtop = (DICTENT *)readblock( dfile );	/* unused symbols */
	string = (char *)readblock( dfile );		/* strings	*/
	dictop = (DICTENT *)string;
	sttop = memtop;				/* top of strings */
}

void readprods() {

	lseek( pfile, 0L, 0 );
	prod = (PRODENT *)readblock( pfile );		/* productions */
	prodtop = (PRODENT *)memtop;		/* top of productions */
}

/*
   scontext - calculates the context set for immediate successors of
   the specified configuration group, and puts the result in the
   specified set.
*/

void scontext(CGRP* cgp, SET* csp) {


	PRODENT	*pp;
	int		el;

	clearset( csp );
	pp = cgp->cpp+prod;
	do {
		pp++;
		el = pp->pel & BM;
		if( pp->pflags & SEM ){		/* end of the production */
			orset( &cgp->cset, csp );
			break;
		}
		if( pp->pflags & NT ){		/* nonterminal symbol */
			orset( el+hs, csp );
			continue;
		}

		/* terminal symbol */

		setbit( el, csp );
		break;
	} while( termtop[el].dflags & MT );
}

/*
   statemove - moves a state nucleus from one place to another.
*/

CGRP* statemove(short* a, short* b, short* lim) {


	short	*top;

	top = (short *)&ST(b)->scg[ST(a)->ssize];
	if( top > lim ) return 0;
	do *b++ = *a++; while( b < top );
	return (CGRP *)top;
}

/*
   writeblock - writes a block of memory to disk, preceded by a word
   giving the length of the block in bytes.  the block to be written
   is specified by a pointer to the first byte, and a pointer to the
   byte following the last byte.
*/

void writeblock(int i, char* first, char* limit) {


	ushort	length;		/* length of the block to be written */

	length = limit - first;
	if( write( i, (char *)&length, sizeof(short) ) != sizeof(short) ||
	    write( i, first, (int)length ) != length )
		fatal( "write error" );
}

/*
   writehs - writes the headsets out to disk.
*/

void writehs() {

	lseek( hfile, 0L, 0 );
	writeblock( hfile, (char *)hs, (char *)hstop );
}

/*
   writestab - writes the stab out to disk.
*/

void writestab() {

	lseek( sfile, 0L, 0 );
	writeblock( sfile, (char *)stab, (char *)stop );
}

/*
   xcomp - comparison routine for sorting xtab.  compares two config-
   groups, first by transition symbol, then by production pointer.
   returns an integer >0, ==0, <0, if the first cgrp is >, ==, < the
   second one, respectively.
*/

void xcomp(CGRP** a, CGRP** b) {


	PRODENT	*app;
	PRODENT	*bpp;
	int		r;

	app = &prod[(*a)->cpp];
	bpp = &prod[(*b)->cpp];
	r = (app->pflags & NT) - (bpp->pflags & NT);
	if( r ) return r;
	r = (app->pel & BM) - (bpp->pel & BM);
	if( r ) return r;
	if( (uint)app > (uint)bpp ) return 1;
	if( (uint)app < (uint)bpp ) return -1;
	return 0;
}


/*VARARGS1*/
void fatal(char* s, ...) {

	va_list argptr;
	va_start(argptr, s);
	fprintf( stderr, "fatal error in usp3: ");
	vfprintf( stderr, s, argptr);
	fprintf( stderr, "\n");
	va_end(argptr);
	rmfiles();
}

void rmfiles() {
	if( !Zflag && !debug ){
		unlink( dname );
		unlink( pname );
		unlink( sname );
		unlink( hname );
		unlink( aname );
		unlink( xname );
	}
	exit(1);
}

int opfile(char* s) {

	int	i;

#ifdef msdos
	_iomode = 1;		/* intermediate files are binary */
#endif
	i = open( s, 2 );
	if( i < 0 ) fatal("cannot open %s",s);
#ifdef msdos
	_iomode = 0;
#endif
	return i;
}
