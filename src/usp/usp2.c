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
*			   Lister					*
*									*
*************************************************************************/


static char rcsid[] =
"@(#)$Header: usp2.c,v 1.3 86/10/08 22:47:12 jdp Exp $";

#include <stdio.h>
#include "usp.h"
#include <fcntl.h> 		/* For 'open' (ES) */
#include <signal.h>
#include <stdarg.h> 		/* For va_arg (ES) */
#include <stdlib.h>		/* For exit (and others) (ES) */
#include <unistd.h> 		/* For 'close' and `sbrk` (ES) */

char	obuf[BUFSIZ];	/* output buffer */
// char	*sbrk();
char	*oname;

DICTENT	*dict;			/* base of dictionary			*/
DICTENT	*termtop;		/* top of terminals			*/
DICTENT	*goaltop;		/* top of goal symbols			*/
DICTENT	*nontermtop;		/* top of nonterminals			*/
DICTENT	*dictop;		/* top of dictionary			*/

char	*string;		/* base of strings			*/
char	*sttop;			/* top of strings			*/

PRODENT	*prod;			/* base of productions			*/
PRODENT	*prodtop;		/* top of productions			*/

char	*memtop;		/* top of occupied memory		*/
char	*memlim;		/* end of acquired memory		*/
char	*membase;		/* base of acquired memory		*/

char	debug;			/* debug flag				*/
char	Zflag;			/* no execl flag			*/

int	dfile;			/* dictionary file			*/
int	pfile;			/* productions file			*/
#ifdef msdos
int	_iomode = 0;
#endif

/*
   Internal function declarations. (ES)
*/
void dumpdict();
void dumpprods();
void dumpsem();
void fatal(char* s, ...);
void grow();
int opfile(char* s);
char* readblock(int i);
void readdict();
void readprods();
void rmfiles();




void intr(){ fprintf(stderr,"usp2 interrupt\n"); rmfiles(); }

int main(int argc, char** argv) {


	char	*flags,	/* pointer to flags */
		*fp;	/* scan pointer for flags */

	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
	if( argc < 3 )fatal("argc error");
	oname = argv[2];
	flags = fp = argv[1];
	if( oname[0] == '-' ){
		oname = argv[1];
		flags = fp = argv[2];
	}
	while( * ++fp ){ /* crack the flags */
		switch(*fp ){

	case 'd':	debug = 1; break;
	case 'Z':	Zflag = 1; break;
		}
	}
	setbuf( stdout, obuf );
	fprintf( stderr, "usp2:	%s\n",oname );
	memtop = memlim = membase = sbrk( 0 );
	dfile = opfile( dname );
	pfile = opfile( pname );
	readdict();
	readprods();
	dumpdict();
	dumpsem();
	dumpprods();

	/* chain to the next phase.  */

	fflush( stdout );
	close( dfile );
	close( pfile );
	if( Zflag ) exit(0);
#ifdef msdos
	exit(0);
#else
	execl( "usp3", "usp3", flags, oname, 0 );
	execl( "/usr/lib/usp3", "usp3", flags, oname, 0 );
	fatal( "cannot reach usp3" );
#endif
}

void dumpdict(){

	DICTENT	*dp;

	printf( "dictionary dump:\n\n" );
	for( dp = dict;dp < dictop;dp++ ){
		if( dp == dict && termtop > dict )
			printf( "\nterminal symbols:\n\n" );
		if( dp == termtop && goaltop > termtop )
			printf( "\ngoal symbols:\n\n" );
		if( dp == goaltop && nontermtop > goaltop )
			printf( "\nnonterminal symbols:\n\n" );
		if( dp == nontermtop && dictop > nontermtop )
			printf( "\nunused symbols:\n\n" );
		printf( " %3d %5d %3o %s\n", dp-dict, dp->dlink,
		dp->dflags&BM, dp->dstring+string );
	}
}

void dumpprods(){

	DICTENT	*dp,
			*dp2;
	PRODENT	*pp;
	short		px;
	short		upel;
	short		oldlp;

	printf( "\nproductions:\n\n" );
	oldlp = -1;
	for( dp = termtop; dp < nontermtop; dp++ ){
		px = dp->dlink;
		while( px ){
			pp = &prod[px];
			px = pp->plink;
			if( (pp->pflags & POS) == 0 ){ /* left side */
				upel = pp->pel & BM;
				if( upel == oldlp )
				    printf( "\t| " );
				else
				    printf( "%s ::= ",
				     &string[termtop[upel].dstring]);
				oldlp = upel;
				while( !((++pp)->pflags & SEM) ){
					upel = pp->pel & BM;
					dp2 = pp->pflags & NT ?
						&termtop[upel] :
						&dict[upel];
					printf( "%s ", &string[dp2->dstring]);
				}
				printf( "%d\n", pp->pel & BM );
			}
		}
		printf( "\n" );
	}
}

/*
   dumpsem - Lists the unused semantic numbers.
*/
void dumpsem() {

DICTENT	*dp;
PRODENT	*pp;
int		i;
int		px;
int		sem;
int		sem1, sem2;
short		semset[256/16];
int		firsttime;

	for(i = 0;  i < 256/16;  i++)	/* mark all sem #'s unused */
		semset[i] = 0;
	for(dp = termtop;  dp < nontermtop;  dp++) {	/* mark used sem #'s */
		px = dp->dlink;
		while(px) {
			pp = px+prod;  px = pp->plink;
			if((pp->pflags&POS) != 0)	/* not left side */
				continue;
			while(!((++pp)->pflags & SEM))	/* scan to sem # */
				;
			sem = pp->pel & BM;		/* semantic number */
			semset[sem>>S] |= 1 << (sem&M);
		}
	}
	printf("\nunused semantic numbers:\n\n   ");
	sem1 = firsttime = 1;
	for( ; ; ) {
		while(sem1 < 256 && semset[sem1>>S] & 1<<(sem1&M))
			sem1++;
		if(sem1 >= 256)
			break;
		sem2 = sem1;
		do
			sem2++;
		while(sem2 < 256 && !(semset[sem2>>S] & 1<<(sem2&M)));
		if(firsttime)
			firsttime = 0;
		else
			putchar(',');
		printf("%d", sem1);
		if(sem2 > sem1+1)
			printf("-%d", sem2-1);
		sem1 = sem2;
	}
	putchar('\n');
}

void grow(){

	if( sbrk( 2048 ) == (char *)-1 ){
		fprintf( stderr, "out of memory in usp2\n" );
		exit( 1 );
	}
	memlim += 2048;
	if( debug ) fprintf( stderr, "growing by 2k bytes\n" );
}

char* readblock(int i) {


	short	length;
	short	al;
	char	*oldtop;

	al = read( i, (char *)&length, sizeof(short) );
	if( al != sizeof(short) )
		fatal( "blocklength read error, al = %d", al );
	while( memtop+length > memlim ) grow();
	al = read( i, memtop, (int)length );
	if( al != length ) fatal( "format error, al = %d", al );
	oldtop = memtop;
	if( length & 1 ) length++;
	memtop += length;
	return oldtop;
}

void readdict(){

	lseek( dfile, 0L, 0 );
	dict = (DICTENT *) readblock( dfile );		/* terminals */
	termtop = (DICTENT *) readblock( dfile );	/* goal symbols */
	goaltop = (DICTENT *) readblock( dfile );	/* nonterminals */
	nontermtop = (DICTENT *) readblock( dfile );	/* unused symbols */
	dictop = (DICTENT *) readblock( dfile );	/* strings */
	string = (char *) dictop;
	sttop = (char *) memtop;			/* top of strings */
}

void readprods(){

	lseek( pfile, 0L, 0 );
	prod = (PRODENT *) readblock( pfile );		/* productions */
	prodtop = (PRODENT *) memtop;			/* top of productions */
}

/*VARARGS1*/
void fatal(char* s, ...) {

	va_list argptr;
	va_start(argptr, s);
	fprintf(stderr,"fatal error in usp2: ");
	vfprintf(stderr,s,argptr);
	fprintf(stderr,"\n");
	va_end(argptr);
	rmfiles();
}

void rmfiles(){
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
	_iomode = 1;
#endif
	i = open( s, 0 );
	if( i < 0 ) fatal("cannot open %s",s);
#ifdef msdos
	_iomode = 0;
#endif
	return i;
}
