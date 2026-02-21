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
*		    Unidot Syntax Processor				*
*			   USP 4					*
*			Build Action Tables				*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: usp4.c,v 1.3 86/10/08 22:47:24 jdp Exp $";

#include <stdio.h>
#include "usp.h"
#include <signal.h>

DICTENT	*dict;		/* base of dictionary			*/
DICTENT	*termtop;	/* top of terminals			*/
DICTENT	*goaltop;	/* top of goal symbols			*/
DICTENT	*nontermtop;	/* top of nonterminals			*/
DICTENT	*dictop;	/* top of dictionary			*/

char	*string;	/* base of strings			*/
char	*sttop;		/* top of strings			*/

char	*oname;		/* input name				*/

#define LI(a)	((LIST *)(a))
PRODENT	*prod;		/* base of productions			*/
PRODENT	*prodtop;	/* top of productions			*/

STATE	*stab;		/* base of state table			*/
#define ST(a)	((STATE *)(a))	/* cast to a state pointer	*/
#define st(a)	((STATE *)((a) + (short *)stab))	/* stab ptr */
#define sx(a)	((short *)(a) - (short *)(stab))	/* stab index */
STATE	*cstate;	/* pointer to current state		*/
STATE	*fstate;	/* pointer to first state		*/
STATE	*stop;		/* top of state table			*/

AENT	*atab;		/* base of action table			*/
#define AT(a)	((AENT *)(a))	/* cast to a action tbl ptr	*/
#define Ap(a)	((AENT *)((a) + (short *)atab))	/* atab ptr	*/
#define Ax(a)	((short *)(a) - (short *)(atab)) /* atab index	*/
AENT	*fact;		/* first action table entry		*/
AENT	*cact;		/* current action table entry		*/
AENT	*atop;		/* top of action table			*/
short	cax;		/* relative current state pointer	*/

int	dfile;		/* dictionary file			*/
int	pfile;		/* productions file			*/
int	sfile;		/* state file				*/
int	hfile;		/* head sets file			*/
int	afile;		/* actions file				*/
int	xfile;		/* alternates file			*/

SET	*hs;		/* base of headsets			*/
SET	*hstop;		/* top of headsets			*/

ALTENT	alttab[ALTSIZE]; /* for recording alternate sem. uses	*/
ALTENT	*alttop;	/* top of alttab			*/

char	*memtop;	/* top of occupied memory		*/
char	*membase;	/* start of occupied memory		*/
char	*memlim;	/* top of acquired memory		*/
char	*sbrk();	/* memory getter			*/
char	*readblock();	/* file reader				*/
char	obuf[BUFSIZ];	/* output buffer			*/

short	alist;		/* action table listing flag		*/
short	Zflag;		/* noexecl flag				*/
short	debug;		/* debug flag				*/
short	ocol;		/* output column position		*/

CGRP	*closure();	/* external declaration			*/
long	lseek();
#ifdef msdos
int	_iomode = 0;
#endif

/*DEB*/char *sy(fl,sym){
static char sem[8];
sym &= 0xff;
if( fl & SEM ){ sprintf(sem,"#%d",sym & 0xff); return sem; }
if( fl & NT ) return dict[sym].dstring+string;
return termtop[sym].dstring+string;
}
/*DEB end*/

intr(){ fprintf(stderr,"usp4 interrupt\n"); rmfiles(); }

main( argc, argv ) int argc; char **argv;{


	reg char	*flags,		/* pointer to flags */
			*fp;		/* for scanning flags */

	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
	if( argc < 3 )fatal("argc");
	oname = argv[2];
	flags = fp = argv[1];
	if( oname[0] == '-' ){
		oname = argv[1];
		flags = fp = argv[2];
	}
	while( *++fp ) switch(*fp ){		/* crack the flags */
case 'd':	debug = 1; continue;
case 'a':
case 'v':	alist = 1; continue;
case 'Z':	Zflag = 1; continue;
	}
	setbuf( stdout, obuf );
	if( debug ) setbuf( stdout, 0 );
	fprintf( stderr, "usp4:	%s\n",oname );
	memtop = memlim = membase = sbrk( 0 );
	dfile = opfile( dname, 0 );
	readdict();
	close( dfile );
	pfile = opfile( pname, 0 );
	readprods();
	close( pfile );
	hfile = opfile( hname, 0 );
	readhs();
	close( hfile );
	sfile = opfile( sname, 0 );
	readstab();
	close( sfile );
	afile = opfile( aname, 2 );
	xfile = opfile( xname, 2 );
	buildatab();
	recalts();
	otheralts();
	adjalts();
	listalts();
	rmdupr();
	rmlr0();
	rmchain();
	mkreach();
	sortatab();
	if( alist ) listatab();
	writealts();
	writeatab();
	close( afile );
	close( xfile );

	/* chain to the next phase.  */

	fflush( stdout );
#ifdef msdos
	exit(0);
#else
	if( Zflag ) exit(0);
	execl( "usp5", "usp5", flags, oname, 0 );
	execl( "/usr/lib/usp5", "usp5", flags, oname, 0 );
	fatal( "cannot reach usp5" );
#endif
}

/*
   acomp - compares two xlist/rlist entries according to the criteria
   listed in sortatab.
*/

acomp( a, b ) reg LIST *a,*b;{


	reg DICTENT	*dbase;
	short		asym;
	short		bsym;
	short		r;

	if( (r = (a->xflags & NT) - (b->xflags & NT)) != 0 ) return r;
	dbase = a->xflags & NT ? termtop: dict;
	asym = a->xsym & BM;
	bsym = b->xsym & BM;
	if( (r = (dbase[asym].dfreq & BM) - (dbase[bsym].dfreq & BM)) != 0 )
		return r;
	return asym-bsym;
}

#ifdef DEBUG
/* showrsem - a debugging procedure*/

showrsem(){

	reg		ax;
	reg LIST	*xp;
	reg LIST 	*top;

	printf( "rsem list***************************\n" );

	ax = Ax(fact);

	do{
		cact = Ap(ax);
		if( ax == 284 ){
			top = &cact->act[cact->nall];
			for( xp = &cact->act[0]; xp < top; xp++ )
				printf( "xp->rsem=%o  xp->xsuc=%o xp->rlp=%o\n",
					xp->rsem, xp->xsuc, xp->rlp );
		}
	} while( (ax = cact->alink) != 0 );
}
#endif


/*
   adjalts - adjusts the alttab such that all entries for a given
   semantic routine return the same left part, i.e. the minimum
   left part of all entries for that semantic number.
*/

adjalts(){

	reg ALTENT	*altp;
	reg ALTENT	*sp;
	char		minlp;
	char		sem;
	char		spanerr;

	for( altp = alttab; altp < alttop; altp++ ){
		minlp = altp->alp;
		sem = altp->asem;
		spanerr = 0;
		for( sp = altp+1; sp < alttop; sp++ ) /* find minimum lp */
			if( sp->asem == sem && ( sp->alp&BM )<( minlp&BM ))
				minlp = sp->alp;
		for( sp = altp; sp < alttop; sp++ ) /* adjust alttab entries */
			if( sp->asem == sem )
				while( (sp->alp&BM) > (minlp&BM) ){
					if( sp->abits&1 ) spanerr = 1;
					sp->abits >>= 1;
					sp->alp--;
				}
		if( spanerr )
			printf("nonterminal span too great for semantic # %d\n",
				sem&BM );
	}
}

/*
   buildatab - builds the action table from the state table.
*/

buildatab(){

	reg LIST	*xp;		/* walks thru action table entries */
	reg AENT	*base;		/* base of current entry	*/
	reg AENT	*top;		/* top of the entry		*/
	short		csx;		/* relative state pointer	*/
	long		fpos;		/* file position		*/
	CGRP		*ctop;		/* pointer to top of closure	*/
	short		blkcnt;		/* number of blocks written	*/
	short		clo[CLOSIZE];	/* area for closing states	*/

	/* first build atab one state at a time, writing directly to afile. */

	lseek( afile, 0L, 0 );
	cax = 0;
	csx = sx(fstate);
	do {
		cstate = st(csx);
		csx = cstate->slink;
		if( !statemove(cstate, (STATE *)clo, clo+CLOSIZE) ||
		    (ctop = closure( (STATE *)clo, (CGRP *)(clo+CLOSIZE))) == 0)
			fatal( "CLOSIZE too small" );
		fixamb( (STATE *)clo, ctop );
		makeact( (STATE *)clo, ctop );
	} while( csx );

	/* now fix the successor pointers on afile */

	lseek( afile, 0L, 0 );
	blkcnt = 0;
	do {
		fpos = lseek( afile, 0L, 1 );
		base = (AENT *)readblock( afile );
		top = (AENT *)memtop;

		for( xp = &base->act[base->nr]; xp < LI(top); xp++ )
			xp->xsuc = st(xp->xsuc)->slink;
		lseek( afile, fpos, 0 );
		writeblock( afile, (char *)base, (char *)top );
		blkcnt++;
		memtop = (char *)base;
	} while( base->alink );

	/* finally, reclaim the storage occupied by the productions,
	   headsets, and state table,
	   then read the action table into memory.  */

	memtop = (char *)prod;			/* reclaim some memory */
	atab = fact = (AENT *)memtop;
	lseek( afile, 0L, 0 );
	while( --blkcnt >= 0 ) readblock( afile );
	atop = (AENT *)memtop;
}

/*
   fixamb - is called with pointers to the base and top of a closed
   state.  resolves and removes all shift-reduce conflicts in the
   state.
*/

fixamb( c, ctop ) STATE *c; CGRP *ctop;{


	reg LIST	*xlp;	/* for walking through xlist		*/
	reg LIST	*mp;	/* for moving xlist entries		*/
	reg PRODENT	*rpp;	/* reduction production pointer		*/
	reg PRODENT	*xpp;	/* transition production pointer	*/
	reg DICTENT	*dp;	/* for looking dictionary entries	*/
	reg CGRP	*rp;	/* for scanning for reductions		*/
	reg CGRP	*xp;	/* for scanning for transitions		*/
	short		rprec;	/* precedence associated with reduce	*/
	short		xprec;	/* precedence associated with shift	*/
	char		sym;	/* the troublesome terminal symbol	*/
	char		shift;	/* flag for keeping the shift		*/
	char		reduce;	/* flag for keeping the reduce		*/
	char		unary;	/* flag for unary operator		*/
	char		listed;	/* flag for avoiding multiple listings	*/

	listed = 0; /* this state not listed yet */
	for( rp = &c->scg[0]; rp < ctop; rp++ ){
		rpp = &prod[rp->cpp];
		if( !(rpp->pflags & SEM) ) continue; /* not reduce */

		/* find the precedence associated with the reduce action */

		do rpp--; while( rpp->pflags & NT && rpp->pflags & POS );

		rprec = 0;		/* clear precedence */
		if( rpp->pflags & POS ){ /* found a terminal */
			dp = &dict[rpp->pel & BM];
			if( (rpp->pflags & POS) == 1 || (rpp+1)->pflags & SEM){
				unary = 1;
				rprec = dp->duprec;
			} else {		/* binary */
				unary = 0;
				rprec = dp->dbprec;
			}
		}

		/* now look for conflicts */

		for( xlp = LI(st(c->xlink));
		     !(xlp->xflags & (XEND|NT)); xlp++ ){

			if( !testbit( xlp->xsym & BM, &rp->cset) )
				continue;

			/* we have found a shift-reduce conflict */

			sym = xlp->xsym;
			xp = &c->scg[0];
			while( prod[xp->cpp].pel != sym ||
			       prod[xp->cpp].pflags & (NT|SEM) ) xp++;
			xpp = &prod[xp->cpp];

			/* find precedence associated with the shift action */

			dp = &dict[sym & BM];
			if( (xpp->pflags & POS ) == 1 ||
			    (xpp+1)->pflags & SEM)		/* unary */
				xprec = dp->duprec;
			else					/* binary */
				xprec = dp->dbprec;

			/* now check the precedences and resolve
			   the conflict accordingly */

			shift = reduce = 0;
			if( rprec && xprec ){ /* both precedences specified */
				if( xprec < rprec ) shift = 1; else
				if( rprec < xprec ) reduce = 1; else
				if( unary ){
					if( dp->dflags&RL )	shift = 1;
						else		reduce = 1;
				} else {	/* binary, precs. equal */
					if( dp->dflags & RA ) shift = 1; else
					if( dp->dflags & LA ) reduce = 1;
				}
			} else {	/* default to the shift operation */
				shift = 1;
				if( !listed ){ /* list state */
					printf( "\n*** ambiguous state:\n" );
					liststate( c, ctop );
					printf( "\n" );
					listed = 1;
				}
				printf( "defaulting to shift on symbol %s\n",
						dp->dstring+string );
			}
			if( !reduce ) clearbit( sym & BM, &rp->cset );
			if( !shift ){	/* remove the xlist entry */
				for( mp = xlp; !(mp->xflags & XEND); mp++ )
					*mp = *(mp+1);
				xlp--;
			}
		}
	}
}

/* grow - expand memory by 2k bytes.  */

grow(){

	reg char *base;

	if( (base = sbrk( 2048 )) == (char *)-1 ) fatal( "out of memory" );
	if( base != memlim )
		fatal( "hole in memory, memlim = 0x%x, base = 0x%x",
		memlim, base );
	memlim += 2048;
	if( debug ) fprintf( stderr, "growing by 2k bytes\n" );
}

/* listalts - lists semantic resolutions required.  */

listalts(){

	reg ALTENT	*altp;
	reg DICTENT	*dp;
	reg		bvec;

	if( alttop <= alttab ) return; /* nothing to list */
	printf( "\n*** semantic resolutions required ***\n\n" );
	for( altp = alttab; altp < alttop; altp++ ){
		dp = ( altp->alp&BM )+termtop;
		printf( "semantic # %d resolves %s to ", altp->asem & BM,
			dp->dstring+string );
		bvec = altp->abits;
		while( bvec ){
			if( bvec & 0x8000 )
				printf( "%s ", dp->dstring+string );
			dp++;
			bvec <<= 1;
		}
		printf( "\n" );
	}
}

/*
   listatab - prints out the action table in a readable format.
*/

listatab(){

	reg DICTENT	*dp;
	reg LIST	*xp;
	reg LIST	*top;
	reg		ax;

	printf( "\naction table list:\n\n" );
	ax = Ax(fact);
	do {
		cact = Ap(ax);
		if( (cact->nr & BM) != BM ) continue;	/* unused state */
		printf( "state %d:\n", ax );
		top = &cact->act[cact->nall];
		for( xp = &cact->act[0]; xp < top; xp++ ){
			if( xp->xflags&NT ) dp = ( xp->xsym&BM )+termtop;
			else dp = ( xp->xsym&BM )+dict;
			printf( "%s ", dp->dstring+string );
			if( xp->xflags&NEWST ){		/* transition */
				printf( "==> %d\n", xp->xsuc );
			} else {			/* reduction */
				printf( xp->xflags&RD ? "*" : " " );
				printf( "%d ", xp->xflags&POS );
				printf( "(%d) ", xp->rsem&BM );
				dp = ( xp->rlp&BM )+termtop;
				printf( "%s\n", dp->dstring+string );
			}
		}
		printf( "\n" );
	} while( (ax = cact->alink) != 0 );
}

/*
   listcg - lists a config-group, including the dotted production and
   the context set.
*/

listcg( cgp ) CGRP *cgp;{


	reg DICTENT	*dp;
	reg PRODENT	*pp;
	reg PRODENT	*dot;

	/* first list the dotted production.  */

	dot = &prod[cgp->cpp];
	pp = dot - (dot->pflags & POS);
	do {
		dp = dict;
		if( pp->pflags & NT ) dp = termtop;
		dp += pp->pel & BM;
		printf( "%s ", dp->dstring+string );
		pp++;
		if( (pp->pflags & POS) == 1 ) printf( "::= " );
		if( pp == dot ) printf( ". " );
	} while( !(pp->pflags & SEM) );
	printf( "%d\n", pp->pel&BM );
	listset( &cgp->cset );
}

/*
   listset - lists the members of the specified set.
*/

listset( s ) SET *s;{


	reg DICTENT	*dp;
	reg char	*sp;
	reg		t;
	char		first;

	first = 1;
	ocol = 1;
	putch( '{' );
	for( t = 0, dp = dict; dp < termtop; t++, dp++ ){
		if( testbit( t, s )){ /* found a member */
			if( !first ) putch( ' ' );
			sp = dp->dstring+string;
			if( ocol+strlen( sp ) > RMARG ) putch( '\n' );
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

liststate( s, top ) STATE *s; reg CGRP *top;{


	reg CGRP	*cgp;

	for( cgp = &s->scg[0]; cgp < top; cgp++ ) listcg( cgp );
}

/*
   makeact - is called with pointers to the base and top of a closed
   state, whose address is in cstate.  builds an action table entry
   from the state and writes it to afile.
*/

makeact( c, ctop ) STATE *c; STATE *ctop;{


	reg PRODENT	*rpp;
	reg LIST	*xlp;
	reg LIST	*top;
	reg CGRP	*rp;
	AENT		*base;
	short		*next;
	short		sym;
	char		f;
	char		lp;
	char		sem;

	base = (AENT *)memtop;

	memtop = (char *)&base->act[0];
	if( memtop > memlim ) grow();

	base->nr = base->nall = base->alink = 0;
	top = LI(memtop);

	/* first put in all the reduction actions */

	for( rp = &c->scg[0]; rp < (CGRP *)ctop; rp++ ){
		rpp = &prod[rp->cpp];

		if( !(rpp->pflags & SEM) ) continue; /* not a reduction */
		f = rpp->pflags & POS;

		lp = (rpp-f)->pel;
		f--;

		sem = rpp->pel;
		for( sym = 0; sym < termtop-dict; sym++ ){
			if( !testbit( sym, &rp->cset ) ) continue;

			memtop = (char *)(top+1);
			if( memtop > memlim ) grow();
			top->rsym = sym;
			top->rflags = f;
			top->rlp = lp;
			top->rsem = sem;
			top = LI(memtop);
			base->nr++;
			base->nall++;
		}
	}

	/* now put in all the transition actions */

	for( xlp = LI(st(cstate->xlink)); !(xlp->xflags&XEND); xlp++ ){
		memtop = (char *)(top+1);
		if( memtop > memlim ) grow();
		top->xsym = xlp->xsym;
		top->xflags = xlp->xflags|NEWST;
		top->xsuc = xlp->xsuc;
		top = LI(memtop);
		base->nall++;
	}

	/* set link fields and write the state to disk */

	next = (short *)top;
	if( cstate->slink ) base->alink = cax + (next - (short *)base);

	cstate->slink = cax; /* for fixing successor pointers later */
	writeblock( afile, (char *)base, (char *)top );
	cax += next - (short *)base;
	memtop = (char *)base;		/* return the memory we used */
}

/*
   mkreach - mark all reachable states in the action table by putting
   a big number into the nr field, which is no longer needed.
*/

mkreach(){

	reg LIST	*xp;
	reg LIST	*xtop;
	reg		ax;

	fact->nr = BM; /* first state is reachable */
	ax = Ax(fact);
	do {
		cact = Ap(ax);
		xtop = &cact->act[cact->nall];
		for( xp = &cact->act[0]; xp < xtop; xp++ )
			if( xp->xflags & NEWST ) /* transition */
				Ap(xp->xsuc)->nr = BM; /* mark successor */
	} while( (ax = cact->alink) != 0 );
}

/*
   otheralts - checks for other uses of semantic routines with
   entries in alttab, and enters these other uses into alttab
   as well.
*/

otheralts(){

	reg ALTENT	*newalt;
	reg LIST	*rp;
	ALTENT		*oldtop;
	ALTENT		*altp;
	LIST		*rtop;
	short		ax;
	char		sem;

	oldtop = alttop;
	for( altp = alttab; altp < oldtop; altp++ ){
		sem = altp->asem;
		ax = Ax(fact);
		do {
			cact = Ap(ax);
			rtop = &cact->act[cact->nr];
			newalt = 0;
			for( rp = &cact->act[0]; rp < rtop; rp++ ){
				if( rp->rflags & ALT || rp->rsem != sem )
					continue;

				/* we have found an unrecorded
				   alttab candidate */

				if( !newalt ){ /* make new alttab entry */
					if( alttop >= alttab+ALTSIZE )
						fatal( "ALTSIZE too small" );
					newalt = alttop++;
					newalt->asem = sem;
					newalt->alp = rp->rlp;
					newalt->abits = 0x8000;
				}
				rp->rflags |= ALT;
				rp->rlp = newalt-alttab;
			}
		} while( (ax = cact->alink) != 0 );
	}
}

/*
   putch - output the specified character and maintain the output column
   position in ocol.
*/

putch( ch ) char ch;{

	if( ch == '\n' ) ocol = 0;
	putchar( ch );
	ocol++;
}

/*
   putst - output the specified string.
*/

putst( s ) reg char *s;{

	while( *s ) putch( *s++ );
}

char *
readblock( i ){


	ushort	length;
	ushort	al;
	char	*oldtop;

	al = read( i, (char *)&length, sizeof(short) );
	if( al != sizeof(short) )
		fatal( "file %d blocklength read error, al = %d", i, al );
	while( memtop+length > memlim ) grow();
	al = read( i, memtop, (int)length );
	if( al != length )
		fatal( "file %d format error, al = %d len %d", i, al, length );
	oldtop = memtop;
	if( length & 1 ) length++;		/* short align	*/
	memtop += length;
	return oldtop;
}

readdict(){

	lseek( dfile, 0L, 0 );
	dict = (DICTENT *)readblock( dfile );		/* terminals */
	termtop = (DICTENT *)readblock( dfile );	/* goal symbols */
	goaltop = (DICTENT *)readblock( dfile );	/* nonterminals */
	nontermtop = (DICTENT *)readblock( dfile );	/* unused symbols */
	string = readblock( dfile );			/* strings */
	dictop = (DICTENT *)string;
	sttop = memtop;					/* top of strings */
}

readhs(){

	lseek( hfile, 0L, 0 );
	hs = (SET *)readblock( hfile );
	hstop = (SET *)memtop;
}

readprods(){

	lseek( pfile, 0L, 0 );
	prod = (PRODENT *)readblock( pfile );		/* productions */
	prodtop = (PRODENT *)memtop;			/* top of productions */
}

readstab(){

	lseek( sfile, 0L, 0 );
	stab = fstate = (STATE *)readblock( sfile );	/* states */
	stop = (STATE *)memtop;				/* top of states */
}

/*
   recalts - checks each state for multiple uses of the same semantic number,
   but different left parts.  records all such cases in alttab, with a bit
   vector to indicate the legal left parts for that semantic routine.
*/

recalts(){

	reg LIST	*rp;
	reg LIST	*sp;
	reg		ax;
	reg		dlp;
	LIST		*rtop;
	char		lp;
	char		sem;
	char		spanerr;

	alttop = alttab;
	ax = Ax(fact);
	do {
		cact = Ap(ax);
		rtop = &cact->act[cact->nr];

		for( rp = &cact->act[0]; rp < rtop; rp++ ){

			if( rp->rflags & ALT ) continue; /* already done */
			sem = rp->rsem;
			lp = rp->rlp;
			for( sp = rp+1; sp < rtop; sp++ )
				if( sp->rsem == sem && sp->rlp != lp ) break;
			if( sp == rtop ) continue;

			/* we have found a semantic number with
			   multiple left part values */

			if( alttop >= alttab+ALTSIZE )
				fatal( "ALTSIZE too small" );
			alttop->asem = sem;
			alttop->alp = lp;
			spanerr = 0;
			for( sp = rp; sp < rtop; sp++ ){
				if( sp->rsem != sem ) continue;
				dlp = (sp->rlp & BM) - (alttop->alp & BM);
				if( dlp >= 0 )
					if( dlp < 16 )
						alttop->abits |= 1 << (15-dlp);
					else spanerr = 1;
				else {
					do{
						if( alttop->abits & 1 )
							spanerr = 1;
						alttop->abits >>= 1;
						alttop->alp--;
					} while( ++dlp );
					alttop->abits |= 0x8000;
				}
				sp->rflags |= ALT;
				sp->rlp = alttop-alttab;
			}
			alttop++;
			if( spanerr ) printf(
			  "nonterminal span too great for semantic # %d\n",
				sem & BM );
		}
	} while( (ax = cact->alink) != 0 );
}

/* rmchain - removes chain reductions from each state.  */

rmchain(){

	reg LIST	*rp;
	reg LIST	*xp;
	reg 		ax;
	LIST		*base;
	LIST		*top;

	ax = Ax(fact);
	do {

		cact = Ap(ax);
		base = &cact->act[0];
		top = &cact->act[cact->nall & BM];
		for( rp = base; rp < top; rp++ ){

			if((rp->rflags & (NEWST|RD|POS)) != (RD|1) || rp->rsem)
				continue; /* not a candidate */

			for( xp = base; xp < top; xp++ )
				if( xp->xsym == rp->rlp && xp->xflags & NT )
					break;
			if( xp < top ){		/* collapse chain reduction */
				rp->rflags = rp->rflags & NT | xp->xflags & ~NT;
				rp->xsuc = xp->xsuc;
				rp--;
			}
		}
	} while( (ax = cact->alink) != 0 );
}

/*
   rmdupr - checks each state in the action table for duplicate reduce actions
   on the same lookahead symbol.  if the reductions involve the same semantic
   number, one of them is removed without complaint.  if, however, the 
   semantic numbers are different, we have a bonafide reduce-reduce conflict,
   and loud noises are produced.
*/

rmdupr(){

	reg LIST	*rp;
	reg LIST	*sp;
	reg LIST	*mp;
	reg		ax;
	LIST		*rtop;
	LIST		*top;
	char		sym;
	char		sem;

	ax = Ax(fact);
	do {
		cact = Ap(ax);
		rtop = &cact->act[cact->nr];
		for( rp = &cact->act[0]; rp < rtop; rp++ ){
			sym = rp->rsym;
			sem = rp->rsem;
			for( sp = rp+1; sp < rtop; sp++ ){
				if( sp->rsym != sym ) continue;
				if( sp->rsem != sem ){	/* shout */
			printf( "*** reduce-reduce conflict in state %d\n", ax);
			printf( "lookahead = %s for semantic #'s %d and %d\n",
					dict[sym & BM].dstring+string,
					sem & BM, sp->rsem & BM );
			printf( "resolving in favor of semantic # %d\n\n",
					sem & BM );
				}

				cact->nr--;
				cact->nall--;
				rtop--;
				top = &cact->act[cact->nall];
				for( mp = sp; mp < top; mp++ )  /* move down */
					*mp = *(mp+1);
				sp--;
			}
		}
	}
	while( (ax = cact->alink) != 0 );
}

/*
   rmlr0 - walks through the action table checking each state's 
   successors for the lr(0) properties:  no transitions and all
   reduce actions identical.  such lr(0) states are removed by moving
   their reduce actions back to the predecessor state and setting
   the RD (eat the token no matter what it is) flag.
*/

rmlr0(){

	reg LIST	*xp;
	reg LIST	*rp;
	reg		ax;
	AENT		*suc;
	LIST		*xtop;
	LIST		*rtop;
	char		f;
	char		lp;
	char		sem;

	ax = Ax(fact);
	do {
		cact = Ap(ax);
		xtop = &cact->act[cact->nall&BM];
		for( xp = &cact->act[cact->nr&BM]; xp < xtop; xp++ ){
			suc = Ap(xp->xsuc);
			if( suc->nr != suc->nall ) continue; /* transitions */
			f = suc->act[0].rflags & ~NT;
			lp = suc->act[0].rlp;
			sem = suc->act[0].rsem;
			rtop = &suc->act[suc->nr&BM];
			for( rp = &suc->act[1]; rp < rtop; rp++ )
				if( rp->rsem != sem ||
				    (rp->rflags & ~NT) != f ||
				    rp->rlp != lp )
					break;
			if( rp >= rtop ){	/* lr(0) successor */
				xp->rflags = (xp->rflags & NT) | f | RD;
				xp->rlp = lp;
				xp->rsem = sem;
			}
		}
	} while( (ax = cact->alink) != 0 );
}

/*
   scontext - calculates the context set for immediate successors of
   the specified configuration group, and puts the result in the
   specified set.
*/

scontext( cgp, csp ) CGRP *cgp; SET *csp;{


	reg PRODENT	*pp;
	reg		el;

	clearset( csp );
	pp = &prod[cgp->cpp];
	do {
		pp++;
		el = pp->pel&BM;
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
   sortatab - sorts the xlist and rlist entries in each state according
   to the following criteria (most important to least important):
	nonterminals > terminals
	higher dfreq > lower dfreq
	higher dict index > lower dict index
*/

sortatab(){

	reg		ax;
	extern	int	acomp();	/* comparison routine for qsort */

	ax = Ax(fact);
	do {
		cact = Ap(ax);
		if( (cact->nr & BM) != BM ) continue; /* unused state */
		qsort(&cact->act[0], cact->nall&BM, sizeof cact->act[0], acomp);
	} while( (ax = cact->alink) != 0 );
}

/*
   statemove - moves a state nucleus from one place to another.
*/

statemove( a, b, lim ) STATE *a,*b;short *lim;{


	reg short	*sha, *shb;
	reg short	*top;

	sha = (short *)a;
	shb = (short *)b;
	top = (short *)&b->scg[a->ssize];
	if( top > lim ) return 0;
	do *shb++ = *sha++; while( shb < top );
	return 1;
}

/* writealts - writes the alttab out to disk.  */

writealts(){

	lseek( xfile, 0L, 0 );
	writeblock( xfile, (char *)alttab, (char *)alttop );
}

/* writeatab - writes the action table out to disk.  */

writeatab(){

	lseek( afile, 0L, 0 );
	writeblock( afile, (char *)atab, (char *)atop );
}

writeblock( i, first, limit ) char *first, *limit;{


	ushort	length;		/* length of the block to be written */

	length = limit-first;
	if( write( i, (char *)&length, sizeof(short) ) != sizeof(short) ||
	    write( i, first, (int)length ) != length )
		fatal("write error");
}


/*VARARGS1*/
fatal( s, a, b ) char *s; {

	fclose(stdout);
	fprintf(stderr,"fatal error in usp4: ");
	fprintf(stderr,s,a,b);
	fprintf(stderr,"\n");
#ifdef DEBUG		/* Dump core for debugging */
	abort();
#endif
	rmfiles();
}

opfile(s,n) char *s;{

	reg	i;

#ifdef msdos
	_iomode = 1;		/* intermediate files are binary */
#endif
	i = open( s, n );
	if( i < 0 ) fatal("cannot open %s",s);
	if(debug)fprintf(stderr,"file %d is %s\n",i,s);
#ifdef msdos
	_iomode = 0;
#endif
	return i;
}

rmfiles(){
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
