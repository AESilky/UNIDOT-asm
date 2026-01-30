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
*			   USP5						*
*			Final Table Output				*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: usp5.c,v 1.3 86/10/08 22:47:34 jdp Exp $";

#include <stdio.h>
#include "usp.h"
#include <signal.h>

DICTENT	*dict;			/* base of dictionary			*/
DICTENT	*termtop;		/* top of terminals			*/
DICTENT	*goaltop;		/* top of goal symbols			*/
DICTENT	*nontermtop;		/* top of nonterminals			*/
DICTENT	*dictop;		/* top of dictionary			*/

char	*string;		/* base of strings			*/
char	*sttop;			/* top of strings			*/
char	*oname;			/* output file name			*/
char	*readblock();
char	*sbrk();

AENT	*atab;			/* base of action table			*/
#define AT(a)	((AENT *)(a))	/* cast to a action tbl ptr	*/
#define Ap(a)	((AENT *)((a) + (short *)atab))	/* atab ptr	*/
#define Ax(a)	((short *)(a) - (short *)(atab)) /* atab index	*/
AENT	*fact;			/* first action table entry		*/
AENT	*cact;			/* current action table entry		*/
AENT	*atop;			/* top of action table			*/
short	cax;			/* relative current action pointer	*/

ALTENT	*alttab;		/* base of alternate semantic use table */
ALTENT	*alttop;		/* top of alternate semantic use table	*/

#define LI(x)	((LIST *)(x))
LIST	*ntact;			/* base of nt default action table	*/
LIST	*ntatop;		/* top of nt default action table	*/

short	*ptab;			/* base of final parse table		*/
short	*ptop;			/* top of final parse table		*/
short	*statep;		/* ptr to current state in final table	*/
short	*ntdflt;		/* base of final nt default table	*/
short	*ntdtop;		/* top of final nt default table	*/

int	dfile;			/* dictionary file			*/
int	afile;			/* actions file				*/
int	xfile;			/* alternates file			*/

short	semtab[SEMSIZE];	/* final semantic table			*/
short	*semtop;		/* top of semtab			*/
short	*semlook();

char	scantab[SCANSIZE];	/* final character scan table		*/
char	*scantop;		/* top of scantab			*/
char	*sclook();

short	*sxtab;			/* base of final state index		*/
short	*sxtop;			/* top of final state index		*/

char	*memtop;		/* top of occupied memory		*/
char	*memlim;		/* top of acquired memory		*/

short	ocol;			/* output column position		*/

FILE	*tabiop;		/* iop for writing tables		*/

char	obuf[BUFSIZ];		/* output buffer			*/
char	tbuf[BUFSIZ];		/* table output buffer			*/

short	aempty;			/* automatic parsing of empties flag	*/
short	debug;			/* debug flag				*/
short	flist;			/* final table listing flag		*/
short	intoff;			/* integer offsets in ptab flag		*/
short	opt;			/* optimize tables (nt defaults)	*/
short	*fconvert();

#ifdef msdos
int	_iomode = 0;
#endif
char hx[] = "0123456789abcdef";

intr(){ fprintf(stderr,"usp5 interrupt!\n"); rmfiles(1); }

main( argc, argv ) int argc; char **argv;{

	reg char	*fp;			/* for scanning flags */

	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
	if( argc < 3 ) fatal("not enough args");
	oname = argv[2];
	fp = argv[1];
	if( oname[0] == '-' ){
		oname = argv[1];
		fp = argv[2];
	}
	while( *++fp ){
		switch(*fp ){

	case 'd':	debug = 1; break;

	case 'e':	aempty = 1; break;

	case 'f':	flist = 1; break;

	case 'i':	intoff = 1; break;

	case 'O':	opt = 1; break;

	case 'v':	flist = 1; break;
		}
	}
	tabiop = fopen( oname, "w" );
	setbuf( stdout, obuf );
	setbuf( tabiop, tbuf );
	fprintf( stderr, "usp5:	%s\n",oname );
	memtop = memlim = sbrk( 0 );
	dfile = opfile( dname, 0 );
	readdict();
	close( dfile );
	afile = opfile( aname, 0 );
	readatab();
	close( afile );
	xfile = opfile( xname, 0 );
	readalts();
	close( xfile );
	if( opt ) finddflts();
	makefinal();
	if( opt ) makedflt();
	if( flist ) listfinal();
	putptab();
	if( opt ) putdflt();
	putsem();
	putscn();
	putstrings();
	fclose( stdout );
	fclose( tabiop );
	rmfiles( 0 );
}

#ifdef DEBUG
showrsem(){

	int		ax;
	reg LIST	*xp;
	reg LIST	*top;


	ax = Ax(fact);
	do {
		printf( "showrsem: state=%d\n", ax );
		cact = Ap(ax);
		top = &cact->act[cact->nall];

		for( xp = &cact->act[0]; xp < top; xp++ ){
			if( ((xp->rlp << 8) | (xp->rsem)) != xp->xsuc ){
				xp->rlp = xp->xsuc >> 8;
				xp->rsem = xp->xsuc & BM;
			}
			printf( "xsuc=%o\n", xp->xsuc );
		}
	} while( (ax = cact->alink) != 0 );
}
#endif

/*
   aconvert - is called with a pointer to an action table entry.  converts
   that entry to final form by adding to ptab, scantab, and semtab.
*/

aconvert( abase ) AENT *abase;{

	LIST		*ap;
	LIST		*aptop;
	LIST		*defact;
	LIST		*nt;
	LIST		*ntp;
	LIST		*nttop;
	LIST		*t;
	LIST		*tp;
	LIST		*ttop;
	LIST		*maxp;
	LIST		*sp;
	char		*symbase;
	char		*symtop;
	char		*scp;
	short		maxct;
	short		ct;
	short		suc;
	short		f;
	short		defflag;

	/* first move nonterminal actions to top of memory */

	nt = nttop = LI(memtop);
	aptop = &abase->act[abase->nall];
	for( ap = aptop-1; ap >= &abase->act[0]; ap-- ){
		if( !(ap->xflags & NT) ) break; /* ignore terminals */
		memtop = (char *)(nttop+1);
		if( memtop > memlim ) grow();
		*nttop = *ap;
		nttop = LI(memtop);
	}

	if( nttop > nt ){ /* we have some nonterminal actions */

		/* we handle nonterminals in one of two ways,
		   depending on the opt flag */

		if( opt ){ /* use nonterminal default table */
			defflag = 0;
			for( ntp = nt; ntp < nttop; ntp++ ){

				/* delete defaults */

				defact = &ntact[ntp->xsym & BM];
				if( ntp->xflags != defact->xflags ||
				    ntp->xsuc != defact->xsuc )
					continue;
				defflag = 1;
				nttop--;
				for( sp = ntp; sp < nttop; sp++ )  /* delete */
					*sp = *(sp+1);
				memtop = (char *)nttop;
				ntp--;
			}
		} else {

			/* use local default method */

			maxct = 0;
			maxp = nt;
			for( ntp = nt; ntp < nttop; ntp++ ){
				ct = 0;
				f = ntp->xflags;
				suc = ntp->xsuc;
				for( sp = ntp+1; sp < nttop; sp++ )
					if( sp->xflags == f && sp->xsuc == suc)
						ct++;
				if( ct > maxct ){
					maxct = ct;
					maxp = ntp;
				}
			}

			/* delete all copies of the most popular action
			   and put the default entry at the end (lowest address)
			   of the list.  note that for the nonterminals section
			   we always default something, even if there are no
			   duplicates.  this helps to keep scantab small by
			   increasing the likelihood of duplicated scan vectors.
			*/

			f = maxp->xflags;
			suc = maxp->xsuc;
			for( ntp = maxp; ntp < nttop; ntp++ ){

				/* delete all copies */

				if( ntp->xflags != f || ntp->xsuc != suc )
					continue;
				nttop--;
				for( sp = ntp; sp < nttop; sp++ ){

					/* move down */

					*sp = *(sp+1);
				}
				memtop = (char *)nttop;
				ntp--;
			}
			for( ntp = nttop; ntp > nt; ntp-- ){

				/* move up a notch */

				*ntp = *(ntp-1);
			}
			memtop = (char *)(++nttop);
			nt->xsym = 0xff;
			nt->xflags = f;
			nt->xsuc = suc;
		}
		/* copy the symbols to the top of memory, add them to scantab,
		   convert the entries to final form, and stick the scan
		   vector into ptab.  */

		symbase = symtop = memtop;
		ntp = nttop;
		while( ntp > nt ){
			ntp--;
			memtop = symtop+1;
			if( memtop > memlim ) grow();
			*symtop = ntp->xsym;
			symtop = memtop;
		}
		if( opt && defflag ){ /* add default symbol to end */
			memtop = symtop+1;
			if( memtop > memlim ) grow();
			*symtop = 0xff;
			symtop = memtop;
		}
		scp = sclook( symbase, symtop );
		memtop = symbase;
		ptop = fconvert( nt, nttop );
		memtop = (char *)(ptop+1);
		if( memtop > memlim ) grow();
		*ptop = scp - scantab;
		ptop = (short *)memtop;
	}

	/* now do the terminal actions in a similar way */

	statep = ptop;		/* this is the address of the state */
	memtop = (char *)(ptop+1);
	if( memtop > memlim ) grow();
	t = ttop = LI(memtop);	/* leave room for scan vector */
	ptop = (short *)ttop;
	for( ap = &abase->act[0]; ap < aptop; ap++ ){
		if( ap->xflags & NT ) break;	/* ignore nonterminals */
		memtop = (char *)(ttop+1);
		if( memtop > memlim ) grow();
		*ttop = *ap;
		ttop = LI(memtop);
	}

	/* check for terminal default action.  (n.b. no shifts allowed) */

	maxct = 0;
	for( tp = t; tp < ttop; tp++ ){
		if( tp->xflags & (NEWST|RD) ) continue;
		ct = 0;
		f = tp->xflags;
		suc = tp->xsuc;
		for( sp = tp+1; sp < ttop; sp++ )
			if( sp->xflags == f && sp->xsuc == suc ) ct++;
		if( ct > maxct ){
			maxct = ct;
			maxp = tp;
		}
	}

	/* if we found a duplicated action, delete all copies and put the
	   default entry at the end (highest address) of the list.  */

	if( maxct ){ /* some action is duplicated at least once */
		f = maxp->xflags;
		suc = maxp->xsuc;
		for( tp = maxp; tp < ttop; tp++ ){ /* delete copies */
			if( tp->xflags!= f || tp->xsuc!= suc ) continue;
			ttop--;
			for( sp = tp; sp < ttop; sp++ )  /* move down */
				*sp = *(sp+1);
			memtop = (char *)ttop;
			tp--;
		}
		memtop = (char *)(ttop+1);
		ttop->xsym = 0xff;
		ttop->xflags = f;
		ttop->xsuc = suc;
		ttop = LI(memtop);
	}

	/* copy terminal symbols to the top of memory, add them to scantab,
	   convert the entries to final form, and stick the scan vector into
	   ptab.  */
	symbase = symtop = memtop;
	for( tp = t; tp < ttop; tp++ ){
		memtop = symtop+1;
		if( memtop > memlim ) grow();
		*symtop = tp->xsym;
		symtop = memtop;
	}
	if( (symtop[-1] & BM) != 0xff ){ /* no default, add error marker */
		memtop = symtop+1;
		if( memtop > memlim ) grow();
		*symtop = 0xfe;
		symtop = memtop;
	}
	scp = sclook( symbase, symtop );
	memtop = symbase;
	ptop = fconvert( t, ttop );
	memtop = (char *)ptop;
	*statep = scp - scantab;	/* scan vector */
	cact->alink = statep - ptab;	/* for fixing successors later */
}

/*
   fconvert - is called with pointers to the base and top of a group
   of action entries.  converts the entries to final form (i.e. with
   separate semtab) and returns a pointer to the top of the result.
*/

short *
fconvert( base, top ) LIST *base,*top;{


	reg LIST	*ap;
	short		*fp;
	short		*semp;
	short		f;

	ap = base;
	fp = (short *)ap;
	while( ap < top ){
		if( ap->xflags & NEWST )		/* transition */
			*fp = LI(ap)->xsuc | 0x8000;
		else{				/* reduction */
			semp = semlook( ap );
			f = 0;			/* flag bits */
			if( LI(ap)->rflags & RD ) f |= 0x4000;
			if( LI(ap)->rflags & ALT ) f |= 0x2000;
			if( !aempty && LI(ap)->rflags & POS )
				LI(ap)->rflags--; /* decr. pop ct. */
			*fp = (LI(ap)->rflags & POS) << 8 |
					f | (semp-semtab & BM);
		}
		ap++;
		fp++;
	}
	return fp;
}

/*
 * finddflts - finds the most common action for each nonterminal symbol, and
 * places these default actions into the table ntact.  ntact consists of
 * xlist/rlist entries, and it is indexed by the nonterminal symbol number.
 */

finddflts(){

	reg LIST	*sp;	/* scan pointer				*/
	reg LIST	*ap;	/* action pointer			*/
	LIST		*ntap;	/* table entry for current nonterminal	*/
	LIST		*maxpt;	/* ptr to most frequent action for this nt */
	short		nt;	/* nonterminal number			*/
	short		maxct;	/* max count seen so far for this nt	*/

	ntact = ntap = ntatop = LI(memtop);
	for( nt = 0; nt < nontermtop-termtop; nt++ ){
		ntap->xsym = 0;
		ntap->xflags = NEWST|NT;
		cax = ntap->xsuc = Ax(fact);
		do {			/* scan entire action table */
			cact = Ap(cax);
			cax = cact->alink;
			if( (cact->nr & BM) != BM ) continue; /* unused state */
			ap = &cact->act[cact->nall];
			while( --ap >= &cact->act[0] && ap->xflags & NT )
				if( (ap->xsym & BM) == nt ) break;

			if( ap < &cact->act[0] || !(ap->xflags & NT) ) continue;

			for( sp = ntap; sp < ntatop; sp++ ) /* lookup action */
				if( ap->xflags == sp->xflags &&
				    ap->xsuc == sp->xsuc ) break;

			if( sp >= ntatop ){ /* enter new action in table */
				ntatop++;
				if( (char *)ntatop > memlim ) grow();
				sp->xsym = 0; /* count of occurrences */
				sp->xflags = ap->xflags;
				sp->xsuc = ap->xsuc;
			}
			sp->xsym++; /* bump count of action occurrences */
		} while( cax );
		maxct = 0;
		maxpt = ntap;
		for( sp = ntap; sp < ntatop; sp++ ){
			if( (sp->xsym & BM) > maxct ){
				maxct = sp->xsym&BM;
				maxpt = sp;
			}
		}
		*ntap = *maxpt;
		ntap++;
		ntatop = ntap;
	}
	memtop = (char *)ntatop;
}

/* grow - expand memory by 2k bytes.  */

grow(){

	if( sbrk( 2048 ) == (char *)-1 ) fatal( "out of memory in usp5" );
	memlim += 2048;
	if( debug ) fprintf( stderr, "growing by 2k bytes\n" );
}

/*
 * inssx - called with a pointer to a ptab or ntdflt entry.  if the entry
 * is a transition, add the state to the state index table.
 */

inssx( pp ) reg short *pp;{


	reg short	*sxp;
	reg short	*mp;
	reg		sp;

	if( !(*pp & 0x8000) ) return; /* not a transition */
	sp = *pp & 0x7fff;
	if( !intoff ) sp >>= 1;
	sxp = sxtop;
	while( sxp-- > sxtab )		/* scan to below sp's position */
		if( sp > *sxp ) break;
	sxp++;					/* sp belongs here */
	if( sxp < sxtop && *sxp == sp ) return; /* sp already there */
	memtop = (char *)(sxtop+1);
	if( memtop > memlim ) grow();
	for( mp = sxtop; mp > sxp; mp-- ) *mp = *(mp-1);
	*sxp = sp;
	sxtop = (short *)memtop;
}

/*
 * listdflt - list the nonterminal default actions.
 */

listdflt(){

	int	nt;

	printf( "\nnonterminal default actions:\n\n" );
	for( nt = 0; nt < nontermtop-termtop; nt++ ){
		printf( "%5d %-20s ", nt, termtop[nt].dstring+string );
		listfa( ntdflt[nt]);
	}
}

/*
 * listfa - list a final table action, given a final table
 * entry.  The entry may be from ptab or ntdflt.
 */

listfa( fa ) int fa;{


	if( fa & 0x8000 )		/* transition entry */
		printf( "==> %5d\n", fa&0x7fff );
	else {				/* reduction entry */
		printf( fa & 040000 ? "   *" : "    " );
		printf( "(%d) ", semtab[fa&BM]&BM );
		printf( "%2d   ", fa >> 8&POS );
		printf( "%s\n", termtop[semtab[fa&BM]>>8 & BM].dstring+string );
	}
}

/* listfinal - lists the final parsing tables in a readable format.  */

listfinal(){

	reg short	*pp;		/* pointer into ptab */
	reg short	*sp;		/* current state pointer in ptab */
	reg short	*sxp;		/* pointer into state index */
	reg char	*scanp;		/* pointer into scantab */

	printf( "\nfinal table list:\n\n" );
	makesx(); /* make an index of the states */
	pp = ptab+1;
	for( sxp = sxtab; sxp < sxtop; sxp++ ){		/* list the states */
		sp = *sxp + ptab;
		if( pp < sp ){		/* list nonterminal section of state */
			scanp = *(sp-1) + (sp-pp-2) + scantab;
			while( pp < sp-1 ){
				listfl( pp, *scanp & BM, termtop );
				pp++;
				scanp--;
			}
			listsv( pp++ );
		}
		scanp = *sp + scantab;
		listsv( pp++ );
		while( (*scanp & BM) != 0xfe ){	/* list terminal section */
			listfl( pp, *scanp & BM, dict );
			pp++;
			if( (*scanp & BM) == 0xff ) break; /* default */
			scanp++;
		}
		printf( "----------\n" ); /* end of state */
	}
	if( opt ) listdflt();
}

/* listfl - lists one line of the final parse table.  */

listfl( pp, sym, dbase ) short *pp;  DICTENT *dbase;{


	reg		px;
	reg char	*str;

	px = pp - ptab;
	if( !intoff ) px <<= 1;
	str = (sym == 0xff) ? "***": dbase[sym].dstring+string;
	printf( "%5d %-20s ", px, str );
	listfa( *pp );
}

/* listsv - lists a scan vector entry of the final tables.  */

listsv( pp ) short	*pp;{


	reg	px;

	px = pp - ptab;
	if( !intoff ) px <<= 1;
	printf( "%5d scan vector              %5d\n", px,*pp );
}

/*
   makedflt - compresses the nonterminal default action table into its
   final form.  The resulting table is between ntdflt and ntdtop.
 */

makedflt(){

	reg short	*pp;
	reg		sx;

	ntdflt = (short *)ntact;
	ntdtop = fconvert( ntact, ntatop );
	for( pp = ntdflt; pp < ntdtop; pp++ ){ /* fix successor pointers */
		if( !(*pp & 0x8000) ) continue; /* not a transition */
		sx = Ap(*pp & 0x7fff)->alink;
		if( !intoff ) sx <<= 1;
		*pp = sx | 0x8000;
	}
}

/*
   makefinal - converts the action table into the final format, consisting
   of the three arrays ptab, semtab, and scantab.
*/

makefinal(){

	reg short	*pp;
	short		ax;
	short		sx;
	short		first;

	ptab = ptop = (short *)memtop;
	semtop = semtab;
	scantop = scantab;
	memtop = (char *)(ptop+1);
	if( memtop > memlim ) grow();
	*ptop = 0;
	ptop = (short *)memtop; /* future first state pointer */
	ax = Ax(fact);
	first = 1;
	do {
		cact = Ap(ax);
		ax = cact->alink;
		if( !((cact->nr & BM) == BM) ) continue; /* unused state */
		aconvert( cact );
		if( first ){
			ptab[0] = statep-ptab;
			first = 0;
			if( !intoff ) ptab[0] <<= 1;
		}
	} while( ax );
	for( pp = ptab+1; pp < ptop; pp++ ){ /* fix successor pointers */
		if( !(*pp & 0x8000) ) continue; /* not a transition */
		sx = Ap(*pp & 0x7fff)->alink;
		if( !intoff ) sx <<= 1;
		*pp = sx | 0x8000;
	}
}

/*
   makesx - makes an index of all the final states in sxtab.  the entries
   are short integer offsets in ptab of the terminal scan vectors for all
   states, sorted in ascending order.
*/

makesx(){

	short	*pp;		/* pointer into ptab or ntdflt */

	sxtab = sxtop = (short *)memtop;
	memtop = (char *)(sxtop+1);
	if( memtop > memlim ) grow();
	*sxtop = ptab[0];
	if( !intoff ) *sxtop >>= 1;
	sxtop = (short *)memtop;
	for( pp = ptab+1; pp < ptop; pp++ ) inssx( pp );
	if( opt ) /* look in ntdflt too */
		for( pp = ntdflt; pp < ntdtop; pp++ ) inssx( pp );
}

/*
   putch - output the specified character and maintain the output column
   position in ocol.
*/

putch( ch ){


	putc( ch, tabiop );
	if( ch == '\n' ) ocol = 0; else ocol++;
}

/*
 * putdflt - outputs the nonterminal default action table in a form that
 * a C compiler can understand.
 */

putdflt(){

	reg short	*pp;
	reg		n;

	putst( "\nshort ntdflt[] = {\n" );
	n = 0;
	for( pp = ntdflt; pp < ntdtop; pp++ ){
		if( (n & 7) == 0 ) putln( n );
		if( pp != ntdflt ) putch( ',' );
		if( (n & 7) == 0 ) putch( '\n' );
		puthex( *pp );
		n++;
	}
	putst( "};\n" );
}

/* puthex - output the specified number in hex, preceded by a 0.  */

puthex( n ) short n;{

	reg	i;
	reg	j;

	j = n & 0xffff;
	putch( '0' );
	putch( 'x' );
	for( i = 12; i >= 0; i -= 4 ) putch( hx[ (j>>i) & 0xf ] );
}

/* putln - output a table line number in form / * xxx * / */

putln(n){

	putch( '/' );
	putch( '*' );
	putch( n > 0xff ? hx[ (n >> 8) & 0xf ] : ' ' );
	putch( n > 0xf ? hx[ (n >> 4) & 0xf ] : ' ' );
	putch( hx[ n & 0xf ]);
	putch( '*' );
	putch( '/' );
	putch( '\t' );
}

/*
   putptab - output the ptab in a form that a C compiler can understand.
*/

putptab(){

	short	*pp;
	reg	n;
	reg	m;

	putst( "\nshort ptab[] = {\n" );
	n = 0;
	m = 0;
	for( pp = ptab; pp < ptop; pp++ ){
		if( pp != ptab ) putch( ',' );
		if( n >= 8 ){
			putch( '\n' );
			n = 0;
		} else
		if( ocol != 0 ) putch( ' ' );
		if( n == 0 ) putln( m );
		puthex( *pp );
		n++;
		m++;
	}
	putst( " };\n" );
}

/*
   putqs - output the specified string, surrounded by double quotes.
   any double quotes or backslash characters are preceded by a backslash.
*/

putqs( s ) reg char *s;{


	putch( '"' );
	while( *s ){
		if( *s == '"' || *s == '\\' ) putch( '\\' );
		putch( *s++ );
	}
	putch( '"' );
}

/*
   putscn - output the scantab in a form that a C compiler can understand.
*/

putscn(){

	reg char	*sp;
	reg		n;
	reg		m;

	putst( "\nchar scntab[] = {\n" );
	n = 0;
	m = 0;
	for( sp = scantab; sp < scantop; sp++ ){
		if( sp != scantab ) putch( ',' );
		if( n >= 8 ){
			putch( '\n' );
			n = 0;
		} else
		if( ocol != 0 ) putch( ' ' );
		if( n == 0 ) putln(m);
		puthex( *sp & BM );
		n++;
		m++;
	}
	putst( " };\n" );
}

/*
   putsem - output the semtab in a form that a C compiler can understand.
*/

putsem(){

	reg short	*sp;
	reg		n;
	reg		m;

	putst( "\nshort semtab[] = {\n" );
	n = 0;
	m = 0;
	for( sp = semtab; sp < semtop; sp++ ){
		if( sp != semtab ) putch( ',' );
		if( n >= 8 ){
			putch( '\n' );
			n = 0;
		} else
		if( ocol != 0 ) putch( ' ' );
		if( n == 0 ) putln(m);
		puthex( *sp );
		n++;
		m++;
	}
	putst( " };\n" );
}

/* putst - output the specified string.  */

putst( s ) reg char *s;{


	while( *s ) putch( *s++ );
}

/*
   putstrings - output the dictionary strings in a form that a C compiler
   can understand.
*/

putstrings(){

	reg DICTENT	*dp;
	reg char	*sp;
	reg		n;

	/* first output the terminals */

	putst( "\nchar *tsym[] = {\n/*  0*/\t" );
	n = 0;
	ocol = 8;
	for( dp = dict; dp < termtop; dp++ ){
		sp = dp->dstring+string;
		if( dp != dict ) putst( ", " );
		if( ocol+strlen( sp ) > RMARG ){
			putch( '\n' );
			putln( n );
			ocol = 8;
		}
		putqs( sp );
		n++;
	}
	putst( ",0};\n" );

	/* now output the nonterminals */

	putst( "\nchar *ntsym[] = {\n/*  0*/\t" );
	n = 0;
	ocol = 8;
	for( dp = termtop; dp < nontermtop; dp++ ){
		sp = dp->dstring+string;
		if( dp != termtop ) putst( ", " );
		if( ocol+strlen( sp ) > RMARG ){
			putch( '\n' );
			putln( n );
			ocol = 8;
		}
		putqs( sp );
		n++;
	}
	putst( ",0};\n" );
}

/* readalts - reads the alttab file and puts it in memory.  */

readalts(){

	lseek( xfile, 0L, 0 );
	alttab = (ALTENT *)readblock( xfile );
	alttop = (ALTENT *)memtop;
}

/* readatab - reads the action file into memory. */

readatab(){

	lseek( afile, 0L, 0 );
	fact = atab = (AENT *)readblock( afile );
	atop = (AENT *)memtop;
}

char *
readblock( i ) int i;{


	ushort		length;

	ushort		al;
	char		*oldtop;

	al = read( i, (char *)&length, sizeof(short) );
	if( al != sizeof(short) )
		fatal( "blocklength read error, al = %d", al );
	while( memtop+length > memlim ) grow();
	al = read( i, memtop, (int)length );
	if( al != length ) fatal( "format error, al = %d len %d\n", al, length);
	oldtop = memtop;
	if( length & 1 ) length++;
	memtop += length;
	return oldtop;
}

readdict(){

	lseek( dfile, 0L, 0 );
	dict = (DICTENT *)readblock( dfile );		/* terminals	*/
	termtop = (DICTENT *)readblock( dfile );	/* goal symbols */
	goaltop = (DICTENT *)readblock( dfile );	/* nonterminals */
	nontermtop = (DICTENT *)readblock( dfile );	/* unused symbols */
	dictop = (DICTENT *)readblock( dfile );		/* strings	*/
	string = (char *)dictop;
	sttop = memtop;					/* top of strings */
}

/*
   sclook - is called with pointers to the base and top of a string of
   symbols.  searches scantab for a matching string and adds the string
   to scantab if no match is found.  returns a pointer to the scantab
   entry.
*/

char *
sclook( base, top ) char *base,*top;{


	reg char	*scp;
	reg char	*sclim;
	int		len;

	len = top-base;
	sclim = scantop+len;
	for( scp = scantab; scp <= sclim;  scp++ )
		if( scmatch( base, scp, len )) break;
	if( scp > sclim ){ /* add new entry to scantab */
		if( scantop+len > scantab+SCANSIZE )
			fatal( "SCANSIZE too small" );
		scp = scantop;
		while( base < top ) *scantop++ = *base++;
	}
	return scp;
}

/*
   scmatch - is called with pointers to two strings and a length.
   returns true iff the strings are equal.
*/

scmatch( a, b, len ) reg char *a,*b; reg len;{


	do if(*a++ != *b++ ) return 0; while( --len );
	return 1;
}

/*
   semlook - is called with a pointer to an rlist entry.  searches for
   a matching entry in semtab, and adds a new entry if none is found.
   returns a pointer to the entry.
*/

short *
semlook( rp ) reg LIST *rp;{


	reg ALTENT	*altp;
	short		*semp;
	short		s0;
	short		s1;

	if( rp->rflags & ALT ){		/* search for a two word entry */
		altp = alttab + (rp->rlp & BM);
		s0 = ((altp->alp & BM) << 8) | (altp->asem & BM);
		s1 = altp->abits;
		for( semp = semtab; semp < semtop-1; semp++ )
			if( *semp == s0 && *(semp+1) == s1 ) break;
		if( semp >= semtop-1 ){ /* add new entry */
			if( semtop+2 > semtab+SEMSIZE )
				fatal( "SEMSIZE too small" );
			semp = semtop;
			*semtop = s0;
			*( semtop+1 ) = s1;
			semtop += 2;
		}
	} else {			/* search for a one word entry */
		s0 = ((rp->rlp & BM) << 8) | (rp->rsem & BM);
		for( semp = semtab; semp < semtop; semp++ )
			if( *semp == s0 ) break;
		if( semp >= semtop ){ /* add new entry */
			if( semtop+1 > semtab+SEMSIZE )
				fatal( "SEMSIZE too small" );
			*semtop++ = s0;
		}
	}
	return semp;
}

/*VARARGS1*/
fatal(s,a,b) char *s; {

	fprintf(stderr,"fatal error in usp5: ");
	fprintf(stderr, s,a,b );
	fprintf(stderr, "\n");
	rmfiles(1);
}

rmfiles(n){
	if( !debug ){
		unlink( dname );
		unlink( pname );
		unlink( sname );
		unlink( hname );
		unlink( aname );
		unlink( xname );
	}
	exit(n);
}

opfile(s,n) char *s;{

	reg	i;

#ifdef msdos
	_iomode = 1;		/* intermediate files are binary */
#endif
	i = open( s, n );
	if( i < 0 ) fatal("cannot open %s",s);
#ifdef msdos
	_iomode = 0;
#endif
	return i;
}
