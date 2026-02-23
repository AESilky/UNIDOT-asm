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
*			   Pass1					*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: usp.c,v 1.2 86/10/06 10:21:17 rmm Exp $";

#include <stdarg.h> 		/* For va_arg (ES) */
#include <stdio.h>
#include <string.h>		/* strcpy (ES) */
#include "usp.h"
#include <signal.h>
#include <stdlib.h>		/* For exit (and others) (ES) */
#include <fcntl.h> 		/* For 'open' (ES) */
#include <unistd.h> 		/* For 'close' and `sbrk` (ES) */

#define	CONTROL	0
#define	SYMBOL	1
#define	NUMBER	2
#define	POUND	3
#define ARROW	4
#define	BAR	5

#define	END	0
#define TERM	1
#define	NTERM	2
#define	PROD	3
#define LEFT	4
#define RIGHT	5
#define NONA	6
#define UNLR	7
#define UNRL	8
#define FREQ	9

char	flags[30];	/* flags to be passed to subsequent phases	*/
char	oname[20];	/* output name					*/
char	debug;		/* debug flag					*/
char	Zflag;		/* no execl flag				*/
char	glist;		/* grammar listing flag				*/
// char	*sbrk();

char	string[STRINGSIZE];
char	*sttop = &string[0];
char	*stlim = &string[STRINGSIZE];

DICTENT	dict[DICTSIZE];
DICTENT	*termtop = dict;		/* top of terminals		*/
DICTENT	*goaltop = dict;		/* top of goal symbols		*/
DICTENT	*nontermtop = dict;		/* top of nonterminals		*/
DICTENT	*dictop = dict;			/* top of dictionary		*/
DICTENT	*dictlim = &dict[DICTSIZE];	/* end of dictionary		*/

prodent_t	*prod;		/* pointer to base of productions data structure */
prodent_t	*prodtop;	/* top of productions area			*/
prodent_t	*prodlim;	/* limit of memory				*/

char	*cwtab[] ={
	 "*end",
	 "*terminals",
	 "*nonterminals",
	 "*productions",
	 "*left",
	 "*right",
	 "*nonassoc",
	 "*unlr",
	 "*unrl",
	 "*freq",
	0
};

char	ibuf[BUFSIZ],	/* input buffer */
	obuf[BUFSIZ];	/* output buffer */

short	ch = '\n';
short	symtype;
short	colct;
short	linect;
short	errct;
int	dfile;			/* dictionary file		*/
int	pfile;			/* productions file		*/

union {
	short	indxval;
	short	val;
	DICTENT	*pval;
}
	symval;

#ifdef msdos
int	_iomode = 0;
#endif

/*
   Internal function declarations. (ES)
*/
void dlook();
void dmove(DICTENT* start, DICTENT* finish);
int dmt(int nt);
void error(char* msg);
void fatal(char* s, ...);
void makefile(char* name);
void markempties();
void newgs(DICTENT* dp, int pos);
void newsem(int semno, int pos);
void nextch();
void nextch();
int opfile(char* s, int n);
void readinput();
void readprods();
void rmfiles();
void setpels();
void sortdict();
void writeblock(int i, char* first, char* limit);
void writedict();
void writeprods();

/*
   main - phase 1 reads the input file and generates a symbol dictionary
   and a data structure describing the productions.  it writes these
   two data structures onto the files dfile and pfile respectively.
*/

void intr(){ fprintf(stderr,"usp interrupt\n"); rmfiles(); }

int main(int argc, char** argv) {


	char	*ap;	/* argument pointer */
	char	*fp;	/* flags pointer */
	char	*iname;	/* input file name */
	int	i;	/* counter */
	char argstr[64];

	iname = NULL;
	fp = flags;
	*fp++ = '-';
	while( --argc ){ /* crack the arguments */
		ap = *++argv; /* pointer to argument */
		if( *ap == '-' ){ /* flags */
			while( *++ap ){
				switch( *ap ){

			case 'Z':	Zflag = 1; break;
			case 'd':	debug = 1; break;
			case 'g':
			case 'v':	glist = 1; break;
			case 'o':	if( *++ap == 0 ){
						if( --argc < 0 )
							fatal("no objname");
						ap = *++argv;
					}
					strcpy( oname, ap );
					goto next;
				}
				*fp++ = *ap;
			}
		} else {		/* input file name */
			if( iname != NULL )
				fatal("too many file names");
			iname = ap;
		}
next:;
	}

	/* open the input file */

	if( iname == NULL )		/* open the specified input file */
		fatal( "no input file" );
	if( oname[0] == 0 ){
		ap = oname;
		fp = iname;
		while( *ap++ = i = *fp++ )
			if( i == '/' ) ap = oname;
		ap--;
		if( ap-oname > 12 ) ap = &oname[12];
		strcpy( ap, ".c" );
	}
	if( freopen( iname, "r", stdin ) == NULL ) /* error from open */
		fatal( "cannot open %s", iname );

	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/

	/* make the temporary and output files.  */

	makefile( dname );
	makefile( pname );
	makefile( sname );
	makefile( hname );
	makefile( aname );
	makefile( xname );
	dfile = opfile( dname, 1 );
	pfile = opfile( pname, 1 );

	setbuf( stdin, ibuf );
	setbuf( stdout, obuf );

	fprintf( stderr, "usp:\t%s \tinput is %s\n",oname,iname );

	prod = prodtop = prodlim = (prodent_t *) sbrk( 0 );
	prodtop++;		/* avoid relative links of zero */
	strcpy( sttop, "_|_" );
	dlook();
	symval.pval->dflags |= RP;
	readinput();
	if( errct ){
		fprintf( stderr, "%d errors in input\n", errct );
		rmfiles();
	}
	sortdict();
	setpels();
	markempties();
	writedict();
	writeprods();

	/* chain to the next phase */

	fflush( stdout );
	close( dfile );
	close( pfile );

	if( Zflag ) exit(0);

	strcpy( argstr, flags );
	strcat( argstr, " " );
	strcat( argstr, oname );
	if( glist ){		/* invoke the lister */
#ifdef msdos
		i = dosrun( "usp2.exe", argstr );
		if( i == -1 ) i = dosrun( "c:/lib/usp2.exe", argstr);
		if( i == -1 ) fatal( "cannot reach usp2" );
#else
		execl( "usp2", "usp2", flags, oname, 0 );
		execl( "/usr/lib/usp2", "usp2", flags, oname, 0 );
		fatal( "cannot reach usp2" );
#endif
	}

	/* invoke phase 3 */

#ifdef msdos
	i = dosrun( "usp3.exe", argstr );
	if( i == -1 ) i = dosrun( "c:/lib/usp3.exe", argstr);
	if( i == -1 ) fatal( "cannot reach usp3" );
	i = dosrun( "usp4.exe", argstr );
	if( i == -1 ) i = dosrun( "c:/lib/usp4.exe", argstr);
	if( i == -1 ) fatal( "cannot reach usp4" );
	i = dosrun( "usp5.exe", argstr );
	if( i == -1 ) i = dosrun( "c:/lib/usp5.exe", argstr);
	if( i == -1 ) fatal( "cannot reach usp5" );
	exit(0);
#else
	execl( "usp3", "usp3", flags, oname, 0 );
	execl( "/usr/lib/usp3", "usp3", flags, oname, 0 );
	fatal( "cannot reach usp3\n" );
#endif
}

/* makefile - creates a file of the specified name */

void makefile( char *name ) {


	int	i;	/* file descriptor */

	i = creat( name, 0666 );
	if( i < 0 ) fatal( "cannot create %s", name );
	close( i );
}



/*
   dlook - searches the dictionary for a symbol and returns a pointer
   to the dictionary in symval.  the symbol is expected to start at
   the character pointed to by sttop.  if the symbol is not found in
   the dictionary, a new entry is created for that symbol.
*/

void dlook(){

	DICTENT	*dp;

	for( dp = dict; dp < dictop; dp++ ) /* search existing entries */
		if( strcmp(dp->dstring+string, sttop) == 0 ){
			symval.pval = dp;
			return;			/* entry was found */
		}

	/* entry was not found; create a new entry */

	if( dp >= dictlim ) fatal( "DICTSIZE too small" );

	dp->dstring = (short)(sttop-string);
	dp->dfreq = 255;
	symval.pval = dp++;
	dictop = dp;
	while( *sttop++ );
}

/* dmove - move a dictionary entry from source to dest */

void dmove(DICTENT* start, DICTENT* finish) {

	int		c;
	char	*source;
	char	*dest;

	source = (char *) start;
	dest = (char *) finish;

	c = sizeof( DICTENT );
	do *dest++ = *source++; while( --c );
}

/*
   dmt - determine if the specified nonterminal can derive the empty
   string.  the flag CM is used to keep track of those nonterminals for
   which this routine has already been called, to avoid redundant
   calculations.
*/

int dmt( int nt ) {


	DICTENT	*dp;
	prodent_t	*pp;
	int		px;

	dp = nt + termtop;		/* pointer to dictionary entry */
	if( dp->dflags & CM )		/* already have the answer */
		return dp->dflags & MT;

	/* must calculate the result */

	dp->dflags |= CM;		/* mark the entry */
	px = dp->dlink;
	while( px ){
		pp = px + prod;
		px = pp->plink;
		if( (pp->pflags & POS) == 0 ) /* LP of production */
		do {
			pp++;
			if( pp->pflags & SEM ){ /* bingo */
				dp->dflags |= MT;
				return 1;
			}
		} while( (pp->pflags & NT) && dmt( pp->pel & BM ) );
	}
	return 0;
}

/*
   dshuffle - scans the portion of the dictionary between dp and the end,
   reordering the entries such that those which match the specified flags
   are placed at the front of this portion of the dictionary.  the value
   returned is a pointer to the top of the area containing the specified
   dictionary entries (i.e. those matching flags).
*/
DICTENT* dshuffle(DICTENT* dp, char flags) {

	DICTENT	temp,	/* used for swapping around dictionary entries */
		*top,	/* top of area accumulating specified symbols */
		*mp;	/* pointer for moving entries around */

	top = dp;
	while( dp < dictop ){
		if( (dp->dflags & (RP|LP) ) == (flags & (RP|LP) )){
			if( dp > top ){ /* not already in place */
				dmove( dp,&temp );
				mp = dp;
				do dmove( mp-1, mp ); while( --mp > top );
				dmove( &temp, top );
			}
			top++;
		}
		dp++;
	}
	return top;
}


void error(char* msg) {

	printf( "at line %d: %s\n", linect, msg );
	errct++;
}

/*VARARGS1*/
void fatal(char* msg, ...) {

	va_list argptr;
	va_start(argptr, msg);
	fprintf(stderr,"usp fatal error at line %d: ",linect);
	vfprintf(stderr,msg, argptr);
	fprintf(stderr,"\n");
	va_end(argptr);
	rmfiles();
}

void rmfiles(){
	unlink( dname );
	unlink( pname );
	unlink( sname );
	unlink( hname );
	unlink( aname );
	unlink( xname );
	exit(1);
}
/*
   markempties - go through the dictionary for all nonterminals and
   determine which ones can derive the empty string.  set the MT flag
   for all such entries.
*/

void markempties(){

	DICTENT	*dp;
	int		nt;

	for( dp = termtop; dp < nontermtop; dp++ ) /* clear all CM flags */
		dp->dflags &= ~CM;
	for( nt = 0; nt < nontermtop-termtop; nt++ ) /* mark empties */
		dmt( nt );
}

/*
   newp - allocate a new entry in the productions data structure and
   return a pointer to it.  if out of memory, call sbrk to get some
   more.
*/

prodent_t *
newp(){

	prodent_t	*pp;

	if( prodtop >= prodlim ){ /* we need more memory */
		if( sbrk( 2048 ) == (char *)-1 )
			fatal( "out of memory" );
		prodlim = (prodent_t *) sbrk( 0 );
	}
	pp = prodtop;
	prodtop = pp+1;
	return pp;
}

/*
   newgs - add a grammar symbol entry to the productions data structure.
*/

void newgs(DICTENT* dp, int pos) {


	prodent_t	*pp;

	pp = newp();
	pp->pflags = pos;

	/* insert the new entry into the chain of all instances
	   of this grammar symbol */

	pp->plink = dp->dlink;
	dp->dlink = (short)(pp - prod);
}

/*
   newsem - add a semantic number entry to the productions data structure.
*/

void newsem(int semno, int pos) {

	prodent_t	*pp;

	pp = newp();
	pp->pflags = pos | SEM;
	pp->pel = semno;
}

/*
   nextch - reads the next character from the input file and puts it in
   the external variable ch.  the external variables colct and linect are
   updated to reflect the column and line numbers, respectively.
*/

void nextch(){


	if( ch == '\n' ){
		colct = 0;
		linect++;
	}
	ch = getchar();
	colct++;
}

/*
   nextsym - reads the next token from the input file, and returns
   a description of the token in the external variables symtype and
   symval.  comments (lines beginning with "* ") are flushed.
  
   symtype and symval are set as follows:
  
   input token		symtype			symval
   -----------		-------			------
   control word		0 (CONTROL)		index in cwtab
   grammar symbol	1 (SYMBOL)		dictionary pointer
   number		2 (NUMBER)		value of the number
   "#"			3 (POUND)		unused
   "::="		4 (ARROW)		unused
   "|"			5 (BAR)			unused
*/

void nextsym(){

	char *sp;	/* pointer for collecting a string */
	int	bol,	/* flag indicating beginning of line */
		com;	/* flag indicating comments are being flushed */

	do{
		while( ch == ' ' || ch == '\t' || ch == '\n' ) nextch();
		bol = colct == 1;
		com = 0;

		/* we now face the first meaningful character of a token.
		   the flag bol is set if the character was the first
		   on its line */

		if( ch >= '0' && ch <= '9' ){ /* read a number */
			symtype = NUMBER;
			symval.val = 0;
			do {
				symval.val = symval.val * 10 + ch - '0';
				nextch();
			} while( ch >= '0' && ch <= '9' );
		} else
		if( ch == EOF ){	/* end of file */
			symtype = CONTROL;
			symval.indxval = END;
		} else {
			/* we read a character string onto the top of
			   the string area.  we then check the string
			   for special cases. */

			sp = sttop;
			do {
				if( sp >= stlim ) fatal( "STRINGSIZE oflo" );
				*sp++ = ch;
				nextch();
			} while( ch != ' ' && ch!= '\t' &&
				 ch!= '\n' && ch!= EOF );
			*sp++ = 0;

			/* our string now resides in the string area,
			   between sttop and sp.  we now check for
			   control words, comments, and special symbols.  */

			if( bol && *sttop == '*' ){ /* comment or control */
				if(*( sttop+1 )== 0 ){ /* comment */
					com = 1;
					while( ch!= '\n' && ch!= EOF ) nextch();
				} else {		/* control word */
					for( symval.val = 0;
					     cwtab[symval.val];
					     symval.val++ )
						 if( !strcmp(sttop,
							cwtab[symval.val]))
							 break;
					if( cwtab[symval.val]) /* found match */
						symtype = CONTROL;
					else{	/* unknown control word */
						error( "unknown control word" );
						com = 1;
						while( ch!= '\n' && ch!= EOF )
							nextch();
					}
				}
			} else
			if( !strcmp( sttop, "#" )) symtype = POUND; else
			if( !strcmp( sttop, "::=" )) symtype = ARROW; else
			if( !strcmp( sttop, "|" )) symtype = BAR; else {
				/* we have a grammar symbol */
				symtype = SYMBOL;
				dlook();
			}
		}

	} while( com );
}

/*
   readinput - reads the input file and processes it according to the
   control words it contains.
*/

void readinput(){

	int	reading,	/* flag to show when we are finished */
		prec,		/* operator precedence */
		freq;		/* operator frequency */

	prec = freq = 0;
	reading = 1;
	nextsym();
	do {
		if( symtype!= CONTROL ){
			error( "control word expected" );
			do nextsym();
			while( symtype!= CONTROL );
		}
		switch( symval.indxval ){

	case TERM:
	case NTERM:	do nextsym();
			while( symtype == SYMBOL );
			break;

	case LEFT:	prec++;
			nextsym();
			while( symtype == SYMBOL ){
				if( symval.pval->dbprec )
					error( "multiple precedences" );
				else {
					symval.pval->dflags |= LA;
					symval.pval->dbprec = prec;
				}
				nextsym();
			}
			break;

	case RIGHT:	prec++;
			nextsym();
			while( symtype == SYMBOL ){
				if( symval.pval->dbprec )
					error( "multiple precedences" );
				else {
					symval.pval->dflags |= RA;
					symval.pval->dbprec = prec;

				}
				nextsym();
			}
			break;

	case NONA:	prec++;
			nextsym();
			while( symtype == SYMBOL ){
				if( symval.pval->dbprec )
					error( "multiple precedences" );
				else symval.pval->dbprec = prec;
				nextsym();
			}
			break;

	case UNLR:	prec++;
			nextsym();
			while( symtype == SYMBOL ){
				if( symval.pval->duprec )
					error( "multiple precedences" );
				else symval.pval->duprec = prec;
				nextsym();
			}
			break;

	case UNRL:	prec++;
			nextsym();
			while( symtype == SYMBOL ){
				if( symval.pval->duprec )
					error( "multiple precedences" );
				else{
					symval.pval->dflags |= RL;
					symval.pval->duprec = prec;
				}
				nextsym();
			}
			break;

	case FREQ:	nextsym();
			while( symtype == SYMBOL ){
				symval.pval->dfreq = ++freq;
				nextsym();
			}
			break;

	case PROD:	readprods();
			break;

	case END:	reading = 0;
			break;
		}
	} while( reading );
}

/*
   readprods - reads the grammar productions and builds the internal
   data structure which describes the grammar.
*/

void readprods(){

	int	 	pos,	/* position of symbol within a production */
			poundval; /* semantic num generated by pound (#) sign */
	DICTENT	*oldlp;	/* dictionary ptr to most recent left part */

	pos = 0;
	oldlp = 0;
	poundval = 255;
	nextsym();
	while( symtype!= CONTROL ){
		switch( symtype ){

	case SYMBOL:	newgs( symval.pval, pos );
			if( pos++ ) symval.pval->dflags |= RP;
			else {
				symval.pval->dflags |= LP;
				oldlp = symval.pval;

			}
			break;

	case POUND:	if( poundval <= 0 ){
				error( "too many semantic #'s" );
				poundval = 255;
			}
			symval.val = poundval--;

			/* flow thru to next case */

	case NUMBER:	if( pos ){
				if( pos > POS ){
					error( "right part too long" );
					pos = 0;
				}
				newsem( symval.val, pos );
				pos = 0;
			} else
				error( "no left part" );
			break;

	case ARROW:	if( pos != 1 ) error( "misplaced ::=" );
			break;

	case BAR:	if( pos == 0 && oldlp )
				newgs( oldlp, pos++ );
			else
				error( "misplaced |" );
			break;

		}
		nextsym();
	}
	if( pos ) error( "unterminated production" );
}

/*
   setpels - goes through the productions data structure and sets
   the pel fields to the appropriate dictionary index for each
   grammar symbol.  for terminal symbols, pel is set to the dictionary
   index of the symbol, relative to dict.  for nonterminals, pel is
   set to the index relative to termtop, and the NT flag is set in
   pflags.  this allows up to 256 terminals and 256 nonterminals without
   confusion.
*/

void setpels(){

	int		px;	/* index in prod */
	prodent_t	*pp;	/* pointer into prod */
	DICTENT	*dp;	/* pointer into dict */

	for( dp = dict; dp < termtop; dp++ ) /* first the terminals */
		for( px = dp->dlink; px; px = pp->plink ){
			pp = &prod[px];
			pp->pel = (char)(dp - dict);
		}
	for( dp = termtop; dp < nontermtop; dp++ ) /* now the nonterminals */
		for( px = dp->dlink; px; px = pp->plink ){
			pp = &prod[px];
			pp->pflags |= NT;
			pp->pel = (char)(dp - termtop);
		}
}

/*
   sortdict - reorders the dictionary such that terminals are at the
   front, followed by goal symbols, then by the other nonterminals,
   and finally any unused symbols.  importantly, the ordering of symbols
   within each of these classes is unchanged.
*/

void sortdict(){

	termtop = dshuffle( dict, RP );		/* terminals to front */
	if( termtop >= dict+SETSIZE )		/* too many terminals */
		fatal( "SETSIZE too small" );
	goaltop = dshuffle( termtop, LP );	/* then goal symbols */
	if( goaltop == termtop )		/* no goal symbols */
		fatal( "no goal symbol" );
	nontermtop = dshuffle( goaltop, RP|LP ); /* then nonterminals */
}

/*
   writeblock - writes a block of memory to disk, preceded by a word
   giving the length of the block in bytes.  the block to be written
   is specified by a pointer to the first byte, and a pointer to the
   byte following the last byte.
*/

void writeblock(int i, char* first, char* limit ) {


	ushort	length;		/* length of the block to be written */

	length = limit-first;
	if( write( i, (char *)&length, sizeof(short) ) != sizeof(short) ||
	    write( i, first, (int)length ) != length )
		fatal("write error");
}

/*
   writedict - writes the dictionary out to disk.  five blocks are
   written, as follows:
  	block 0 - terminal symbol dictionary entries
  	block 1 - goal symbol dictionary entries
  	block 2 - remaining nonterminal symbol dictionary entries
  	block 3 - unused symbol dictionary entries
  	block 4 - strings
   all blocks are written, even though some (e.g. block 3) may have
   zero length.
*/

void writedict(){

	lseek( dfile, 0L, 0 );
	writeblock( dfile, (char *)dict, (char *)termtop );
	writeblock( dfile, (char *)termtop, (char *)goaltop );
	writeblock( dfile, (char *)goaltop, (char *)nontermtop );
	writeblock( dfile, (char *)nontermtop, (char *)dictop );
	writeblock( dfile, string, sttop );
}

/*
   writeprods - write the productions data structure to disk.  a single
   block is written, which contains the entire data structure.
*/

void writeprods(){

	lseek( pfile, 0L, 0 );
	writeblock( pfile, (char *)prod, (char *)prodtop );
}

int opfile(char* s, int n) {

	int	i;

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
