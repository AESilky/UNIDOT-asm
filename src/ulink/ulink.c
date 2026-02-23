/************************************************************************
*									*
*	Copyright (C) 1982,1985,1987 by Unidot, Inc.			*
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
*			ulink.c - main ulink module			*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: ulink.c,v 4.26 92/07/31 07:58:06 rmm Rel $ main ulink module";

#define VARS 1			/* put the variables in this module */
#define LNKCNT	200		/* max number of files to link		*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
//#include <libexplain/mkstemp.h>
#ifdef vms
#include "[-.incl]uobj.h"
#endif

#ifdef msdos
#include "../incl/uobj.h"
int	_iomode = 1;
#endif

#ifndef NOTUNIX
#include "../incl/uobj.h"
#endif
#include "ulink.h"
#include "funcdefs.h"		/* Forward defines for GCC */

char	symfname[] = "usyXXXXXX";	/* last 6 chars must be 'X' for `mkstemp` */
char	relfname[] = "urlXXXXXX";	/* last 6 chars must be 'X' for `mkstemp` */
char	locfname[] = "ulcXXXXXX";	/* last 6 chars must be 'X' for `mkstemp` */
short	filex;
short	singlecol;
short	nsc;			/* National Semiconductor Corp. flag ('NLINK') */
short	wrnex = WRNEXIT;
char	nil[2];			/* empty string	*/
char	*files[LNKCNT];
long	curtime;
char	datstr[32];
char	date[16];
char	timstr[16];

/* Declarations (local) */

void afinish();
void bigspace(int n);
void copymsg(FILE* f);
void defsym(char* s);
void flushclos(FILE* f);
void linkspec(char* lnkfile);
void map();
void page();
void pent(sytab_t* syp);
void psect(section_t* secp);
void usage();


void intr(){  fprintf(stderr,"INTERRUPT!\n"); quit( FATEXIT ); }
/*		Here starts ulink			*/


void main(int argc, char* argv[], char* env[]) {

	section_t	*secp;

#ifdef vms
	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
#endif
#ifdef msdos
	argc = argsetup( argv, 1000, &argv, argv[0] );
	signal( SIGINT, intr ); /* interrupt            */
#endif
#ifndef NOTUNIX
	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
#endif

	time(&curtime );
	strcpy( datstr, ctime(&curtime ));
	strncpy( timstr, datstr+11, 8);
	strncpy( date, datstr+4, 7 );
	strncpy( date+7, datstr+20, 4 );
	commvar = "commvar";		/* name of unnamed section	*/
#ifdef STATS
	inbuf = palloc( BUFSIZ, BUFUSE );
#else
	inbuf = palloc( BUFSIZ );
#endif
	prname = lastcomp( argv[0] );
	if( prname[0] == 'n' || prname[0] == 'N' ){
		// National Semiconductor 'NLINK'
		nsc++;
		singlecol = 1;
	}
	if( argc <= 1 ) usage();
	init( argc-1, argv+1 );
	if( filex == 0 ) usage();
	if( verbose ) copymsg(stderr);
	if( rflag ) kflag = split = 0;
	if( mapfile == 0 ) mapfile = "a.map";	/* name of map file	*/
	if( objfile == 0 ) objfile = "a.obj";	/* name of object file	*/
	OBJOUT = fopen( objfile, "w" );
	if( OBJOUT == NULL ) nocreat( objfile );
#ifdef STATS
	setbuf( OBJOUT, palloc(BUFSIZ,BUFUSE));
#else
	setbuf( OBJOUT, palloc(BUFSIZ));
#endif
	if( mflag ){
		LIST = fopen( mapfile, "w" );
		if( LIST == NULL ) nocreat( mapfile );
#ifdef msdos
		setmode(fileno(LIST),"text");
#endif
#ifdef STATS
		setbuf( LIST, palloc(BUFSIZ,BUFUSE));
#else
		setbuf( LIST, palloc(BUFSIZ));
#endif
		if( nflag ){
			int f = mkstemp(locfname);
			if( f < 0 ) nocreat( "local symbol file" );
			LOCFILE = fdopen(f, "w");
#ifdef msdos
			setmode(fileno(LOCFILE),"text");
#endif
		}
	} else
		nflag = 0;
	if( afmt ){
		if( !sflag ){
			int f = mkstemp(symfname);
			if( f < 0 ) {
				nocreat("symbol file");
			}
			SYMFILE = fdopen(f, "w");
		}
		if( rflag ){
			int f = mkstemp(relfname);
			if( f < 0 ) nocreat("relocation file");
			RELFILE = fdopen(f, "w");
		}
	}
	if( ovlfile ){		/* building an overlay */
		OVLYFILE = fopen( ovlfile, "r" );
		if( OVLYFILE == NULL )
			noread("overlay control file");
#ifdef msdos
		setmode(fileno(OVLYFILE),"text");
#endif
		setbuf(OVLYFILE,0);
	}
	if( verbose ) printf("pass1:\n");
#ifdef STATS
	sectab[URBABS] = secp = (section_t *)zpalloc( sizeof(section_t), SECUSE );
#else
	sectab[URBABS] = secp = (section_t *)zpalloc( sizeof(section_t) );
#endif
	secp->se_sym = sylook( ".abs  " );		/* NOTE BLANKs	*/
	secp->se_sym->sy_typ = STSEC;			/* no list	*/
	secp->se_atr |= USEABS;				/* absolute	*/
#ifdef STATS
#ifdef DEBUG
prstats("before dopass");
#endif
#endif
	dopass( filex, files );
#ifdef STATS
#ifdef DEBUG
prstats("before interlude");
#endif
#endif
	interlude();
	if( errct == 0 ){ /* no errors detected in pass 1 */
		pass2 = 1;
		if( verbose ) printf("PASS2:\n");
		dopass( filex, files );
		if( afmt ){
			afinish();
		} else {
			objblk.ob_type = UOBOND;
			oflush();
		}
		if( verbose ) printf("end of linking\n");
	}
	flushclos( OBJOUT ); OBJOUT=(FILE*)0;
	if( errct && verbose ) printf("pass 2 not performed\n");
	if( mflag ) map();
	if( ovct ) printf( "%4d overlapping sections\n", ovct );
	if( undct ) printf( "%4d undefined symbols\n", undct );
	if( mulct ) printf( "%4d multiply defined symbols\n", mulct );
	if( errct ){
		printf("%4d linker warnings\n", warnct);
		printf( "%4d linker errors\n", errct );
		quit( BADEXIT );
	}
	if( verbose | warnct) printf("%4d linker warnings\n  no linker errors\n", warnct);
	quit( warnct ? wrnex : GOODEXIT );
}
/*
 * init - Initialization processing.
 */

void init(int argc, char** argv) {

	char	*ap;
	int	i;

	for( i=0; i<argc; i++ ){
		ap = argv[i];
		if( *ap != '-' ){
			if( filex >= LNKCNT ) error("F07 too many files");
			files[filex++] = ap;
			continue;
		}

		/* read command line switches */

		ap++;
		while( *ap ) switch( *ap++ ){

		case 'A':
		case 'a': afmt++;		/* a.out format		*/
			  continue;
		
		case 'B':
		case 'b': binfmt++; afmt++;	/* binary (raw) format (use afmt processing for most) */
			  sflag++;		/* no symbols either */
			  if (*ap) {
				fillb = scanbyte(ap);	/* fill byte specified */
				ap = nil;
			  }
			  continue;

		case '1': singlecol++;		/* single column map	*/
			  continue;

		case '0': if( *ap != 'd' ) usage();
			  while( *ap++ == 'd' ) debug++;
			  verbose++;		/* also, be verbose */
			  ap--;
			  continue;

		case 'D':
		case 'd': defsym( *ap ? ap : argv[++i] ); /* define symbol */
			  ap = nil;
			  break;

		case 'F':
		case 'f': linkspec( *ap ? ap : argv[++i] ); /* link file */
			  ap = nil;
			  break;

		case 'G':
		case 'g': wrnex = GOODEXIT;
			  break;

		case 'I':
		case 'i': split++;		/* split I/D load	*/
			  continue;

		case 'K':
		case 'k': kflag++;		/* keep sections option	*/
			  continue;

		case 'L':
		case 'l': locspec( *ap ? ap : argv[++i] ); /* set address */
			  ap = nil;
			  break;

		case 'M':
		case 'm': mflag = 1;
			  if( *ap == 0 ) continue;
			  if( *ap == '=' ) ap++;
			  if( mapfile ) error("F08 Too many mapfile names");
			  mapfile = ap;
			  ap = nil;
			  break;

		case 'N':
		case 'n': nflag = 1;		/* flag for local syms */
			  continue;

		case 'O':
		case 'o': if( objfile ) error("F09 Too many object file names");
			  if( *ap ) objfile = ap; else objfile = argv[++i];
			  if( *objfile == '=' ) objfile++;
			  ap = nil;
			  break;

		case 'P':
		case 'p': numorder++;		/* numeric order map */
			  continue;

		case 'R':
		case 'r': rflag++;		/* relinking option	*/
			  continue;

		case 'S':
		case 's': sflag++;		/* no symbols	*/
			  continue;

		case 'V':
		case 'v': verbose++;		/* be talky	*/
			  continue;

		case 'Y':
		case 'y': if( ovlfile ) error("F10 Too many overlay file names");
			  if( *ap ) ovlfile = ap; else ovlfile = argv[i++];
			  if( *ovlfile == '=' ) ovlfile++;
			  ap = nil;
			  break;;

		case 'Z':
		case 'z': nomix++;		/* don't mix sects with */
			  continue;		/* different attributes */

		default:  usage();		

		}
	}
	if( i > argc ) usage();
	if( sflag && rflag )
		error("F94 The -b or -s, and -r options may not be used together");
}

void linkspec( char *lnkfile ) {	/* do a link file */

	char		*ap;
	int		i;
	int		ax;
	FILE		*LNKFILE;
	char		tmp[128];
#define LNKSIZ	128
	char		*aav[LNKSIZ];

	if( lnkfile == 0 ) usage();
	if( *lnkfile == '=' ) lnkfile++;
	LNKFILE = fopen(lnkfile,"r");
#ifdef msdos
	setmode(fileno(LNKFILE),"text");
#endif
	if( LNKFILE == NULL ) noread(lnkfile);
	setbuf(LNKFILE,0);
	ax = 0;
	for(;;){
		i = getc(LNKFILE);
		if( i == ';' ) while( i != EOF && i != 032 && i != '\n' )
					i = getc(LNKFILE);
		if( i == EOF || i == 032 ) break;
		if( i == ' ' || i == '\t' || i == '\n' ) continue;
		if( ax >= LNKSIZ ) error("F04 too many linker specs");
		ap = tmp;
		while( i != ' ' && i != '\n' && i != '\t' && i != EOF ){
			*ap++ = i;
			i = getc(LNKFILE);
		}
		*ap++ = 0;
#ifdef STATS
		aav[ax++] = ap = palloc( strlen(tmp)+1, OTHUSE );
#else
		aav[ax++] = ap = palloc( strlen(tmp)+1 );
#endif
		strcpy( ap, tmp );
	}
	aav[ax] = NULLCA;
	flushclos( LNKFILE ); LNKFILE=(FILE*)0;
	init( ax, aav );		/* recurse */
}
/*

 * scanaddr - convert an address
 */
long scanaddr(char* s) {

	long	v;
	int	c;

	v = 0L;
	while( (c = *s++ ) != '\0' ){
		if( '0' <= c && c <= '9' ) c += 0- '0'; else
		if( 'a' <= c && c <= 'f' ) c += 10- 'a'; else
		if( 'A' <= c && c <= 'F' ) c += 10- 'A'; else
			error( "F05 Bad location address" );
		v = (v << 4) | c;
	}
	return v;
}

/*
 * scanbyte - convert a byte
 */
unsigned char scanbyte(char* s) {

	short	v;
	int	c;

	v = 0;
	while ((c = *s++) != '\0') {
		if ('0' <= c && c <= '9') c += 0 - '0'; else
			if ('a' <= c && c <= 'f') c += 10 - 'a'; else
				if ('A' <= c && c <= 'F') c += 10 - 'A'; else
					error("F11 Bad byte value");
		v = (v << 4) | c;
	}
	if (v > 0xff)
		error("F11 Bad byte value");
	return (unsigned char)v;
}
/*
 * locspec - Processes a location specification string.
 */

void locspec(char* s) {


	char	*n;
	section_t	*secp;
	char		name[128];

	n = name;
	while( *s != '\0' && *s != '=' ) *n++ = *s++;	/* copy the name */
	*n = name[32] = NULLCA;
	secp = selook( name );
	if( *s == '=' ){				/* read the address */
		secp->se_val = scanaddr( s+1 );
		secp->se_cum = 0L;
		secp->se_mod = 0L;
		secp->se_atr |= USEFIX;
	}
}
/*
 * defsym - Processes a symbol definition
 */

void defsym( char *s) {


	char	*n;
	sytab_t	*syp;
	char		name[128];

	n = name;
	while( *s != '\0' && *s != '=' ) *n++ = *s++;	/* copy the name */
	*n = name[32] = NULLCA;
	syp = sylook( name );
	if( *s != '=' ) error("F06 Illegal definition");
	syp->sy_val = scanaddr( s+1 );		/* get the value	*/
	syp->sy_atr = SADP2;			/* define in pass2	*/
	syp->sy_typ = STGLO;			/* make it global	*/
	syp->sy_ord = nxtord++;			/* assign in now	*/
}
/*
 * rev - reverse a list
 */

sytab_t* rev(sytab_t* p) {
	sytab_t	*q;
	sytab_t	*r;

	q = 0;			/* last element on chain */
	while( p ) r = p->sy_lnk, p->sy_lnk = q, q = p, p = r;
	return q;
}

static char *symfmt;

void pent(sytab_t* syp) {	/* print a symbol entry	*/

	char	*valfmt;

	fprintf( LIST, symfmt, syp->sy_str );
	if( OVLYFILE ) fprintf( LIST, "%4d", syp->sy_ovl & 0xff );
	valfmt = " %8lx  ";
	if( syp->sy_typ == STUND ) valfmt =  "undefined  "; else
	if( syp->sy_atr & SAMUD )  valfmt =  " multiple  ";
	fprintf( LIST, valfmt, syp->sy_val );
}

void psect(section_t* secp) {	/* print a section entry	*/

	sytab_t	*syp;
	section_t	*sep2;
	int		i = 0;

#define SE_PRINTED 1
	if( secp->se_atr2 & SE_PRINTED ) return;
top:	secp->se_atr2 |= SE_PRINTED;
	if( linect < 1 ) page();
	syp = secp->se_sym;
	if( i ) fprintf( LIST, "%-25.25s ext %2d",syp->sy_str,i);
	   else fprintf( LIST, "%-32.32s", syp->sy_str);
	fprintf( LIST," %8lx %8lx %8lx        %2d         %2d        %2d\n",
		secp->se_val,
		secp->se_val+secp->se_cum,
		secp->se_cum,
		secp->se_aln,
		secp->se_ext,
		secp->se_adu);
	linect--;
	if( secp->se_atr & USEXTD1 ){
		i++;
		secp = sectab[secp->se_xtd & 0xff];
		goto top;
	}
}

void bigspace(int n) {		/* output big space between sections */

	if( linect < n || linect < 3 ){
		page();
	} else {
		linect -= 3;
		fprintf(LIST,"\n\n\n");
	}
}

/*
 * map - Outputs the load map.
 */

void map(){

	sytab_t	*syp;
	int		i;
	sytab_t	*syp2;
	sytab_t		*c1,*c2,*c3;	/* columns for printing */
	section_t	*secp;
	long		curtime;
	char		datstr[64];
	char		timstr[16];
	char		date[16];


	time(&curtime );
	strcpy( datstr, ctime(&curtime ));
	strncpy( timstr, datstr+11, 8);
	strncpy( date, datstr+4, 7 );
	strncpy( date+7, datstr+20, 4 );
	sysort();

	/* List information about the sections.  */

	pghead =
"Section                             Start      End     Size     Align     Extent     Addru";
	if( sectab[0]->se_cum ) psect( sectab[0] );
	for( i = URBSEC; i < stct; i++ ) psect( sectab[i] );
	if( codsep && (codsep->se_cum || datsep->se_cum)){

		/* resulting sections for split id	*/

		bigspace(8);
		linect -= 2;
		fprintf(LIST,"Derived code section for split code/data\n\n");
		codsep->se_val = codsep->se_cum;
		datsep->se_val = datsep->se_cum;
		for( i = URBSEC; i<stct; i++ ){
			secp = sectab[i];
			if( secp->se_atr & USEABS ) continue;
			if( secp->se_atr & USENOX ){
				if( secp->se_val < datsep->se_val )
					datsep->se_val = secp->se_val;
			} else {
				if( secp->se_val < codsep->se_val )
					codsep->se_val = secp->se_val;
			}
		}
		codsep->se_cum -= codsep->se_val;
		datsep->se_cum -= datsep->se_val;
		psect( codsep );
		linect -= 2;
		fprintf(LIST,"Sections comprising derived code section\n\n");
		for( i = URBSEC; i<stct; i++ ){
			secp = sectab[i];
			if( secp->se_atr & (USEABS|USENOX) ) continue;
			psect( secp );
		}
		bigspace(8);
		fprintf(LIST,"Derived data section for split code/data\n\n");
		psect( datsep );
		linect -= 2;
		fprintf(LIST,"Sections comprising derived data section\n\n");
		for( i = URBSEC; i<stct; i++ ){
			secp = sectab[i];
			if( secp->se_atr & USEABS ) continue;
			if( !(secp->se_atr & USENOX) ) continue;
			psect( secp );
		}
	}

	pghead =
"Symbol            Value  Symbol            Value  Symbol            Value";
	symfmt = "%-14.14s";
	if( singlecol ){
		pghead = "Symbol                              Value";
		symfmt = "%-32.32s";
	}
	if( OVLYFILE ){
		pghead =
"Symbol     Ovl    Value  Symbol     Ovl    Value  Symbol     Ovl    Value";
		symfmt = "%-10.10s";
		if( singlecol ){
			pghead = "Symbol                       Ovl    Value";
			symfmt = "%-28.28s";
		}
	}
	i = 0;
	c1 = c2 = c3 = 0;
	for( syp = syhtab[0]; syp; syp = syp2 ){
		syp2 = syp->sy_lnk;
		if( syp->sy_typ == STSEC || syp->sy_typ == STGRP )
			continue;
		if( singlecol ){
			syp->sy_lnk = c1;
			c1 = syp;
			continue;
		}
		if( i < 56 ) syp->sy_lnk = c1, c1 = syp; else
		if( i < 112 ) syp->sy_lnk = c2, c2 = syp; else
			syp->sy_lnk = c3, c3 = syp;
		if( ++i == 3*56 ) i = 0;
	}

	/* the chains are now reversed so turn them around */

	c1 = rev(c1);
	c2 = rev(c2);
	c3 = rev(c3);
	while( c1 != NULL ){
		page();
		for( i=0; i<56 && c1 != NULL; i++ ){
			pent(c1);
			c1 = c1->sy_lnk;
			if( !singlecol ){
				if( c2 ) pent(c2), c2 = c2->sy_lnk;
				if( c3 ) pent(c3), c3 = c3->sy_lnk;
			}
			fputc( '\n', LIST );
		}
	}
	if( LOCFILE ){
		pghead =
"Symbol      Value  Symbol      Value  Symbol      Value  Symbol      Value";
		flushclos( LOCFILE ); LOCFILE=(FILE*)0;
		LOCFILE = fopen( locfname, "r" );
		if( LOCFILE == NULL ) noread("local symbol file");
		linect = 0;
		while( (i = getc(LOCFILE)) != EOF ){
			if( linect < 1 ) page();
			while( i != '\n' && i!= EOF ){
				putc( i, LIST );
				i = getc(LOCFILE);
			}
			putc( '\n', LIST );
			linect--;
		}
	}
	fflush( LIST );
	if( ferror(LIST) ) error("F93 listing file write error");
}

void page(){

	fprintf( LIST,
		"\f\n\n%-16s    %14s  %8s%-*sPage%4d\n\n%s\n\n",
			prname,date,timstr,28, " ", ++pagect, pghead );
	linect = 56;
}

void afinish(){		/* clean up the a.out format module */

	int		i;

DEBOUT(0,("afin fpos = %ld, relsize = %ld, symsize = %ld\n",fpos,relsize,symsize));
	fseek(OBJOUT, fpos, SEEK_SET);	/* move to end of file */
	fclean( fpos, 0 );
	if( rflag ){			/* copy the relocation items */
		flushclos( RELFILE ); RELFILE=(FILE*)0;
		RELFILE = fopen( relfname, "r" );
		if( RELFILE == NULL ) noread("relocation file");
		while( (i = getc(RELFILE)) != EOF ) putc( i, OBJOUT );
		fclose( RELFILE ); // RELFILE=(FILE*)0;
	}
	if( !sflag ){			/* copy the symbols */
		flushclos( SYMFILE ); SYMFILE=(FILE*)0;
		SYMFILE = fopen( symfname, "r" );
		if( SYMFILE == NULL ) noread("symbol file");
		while( (i = getc(SYMFILE)) != EOF ) putc( i, OBJOUT );
		fclose( SYMFILE ); // SYMFILE=(FILE*)0;
	}
	if( !binfmt && (relsize || symsize || tranad) ){
		fseek(OBJOUT, 4L, SEEK_SET);
		along( tranad,  OBJOUT );
		along( relsize, OBJOUT );
		along( symsize, OBJOUT );
	}
}

void fclean(long pos, int length) {

	static long hiwater = 0;

	if( hiwater < pos ){
		fseek(OBJOUT, hiwater, SEEK_SET);
		DEBOUT(0, ("fclean fill (%02X): %ld\n", fillb, pos - hiwater));
		while( hiwater < pos ){
			putc( fillb, OBJOUT );
			hiwater++;
		}
	}
	if( pos + length > hiwater ){
		hiwater = pos + length;
		DEBOUT(0,("fclean: pos:%ld + len:%d = hiwater:%ld\n",pos,length,hiwater));
	}
}

/*
 * usage - Show the options. Issues a fatal error message for a command line error.
 */
void usage(){

	copymsg(stdout);
	printf("Usage: %s [options].. file [file]..\n",prname);
	printf("	-a		produce image output in a.out style\n");
	printf("        -b[n]           produce binary (raw image) output (use n for fill)\n");
	if( !nsc ) {
		printf("	-1		(one) single column map file for symbols\n");
	}
	printf("	-f<file>	<file> is a linker control file\n");
	printf("	-i		split I/D load\n");
	printf("	-l<locspec>	force a section location or order:\n");
	printf("	    <sect>	    section follows previous\n");
	printf("	    <sect>=<addr>   section has specified address\n");
	printf("	-m		produce a load map on 'a.map'\n");
	printf("	-m<file>	produce a load map on <file>\n");
	printf("	-o<file>	name the object <file> not 'a.obj'\n");
	printf("	-r		produce a relinkable module\n");
	printf("	-s		suppress symbol output in object\n");
	printf("	-v		verbose option\n");
	printf("	-y<file>	<file> is an overlay control file\n");
	printf("        -0d[d...]       set debug level to 'd' count\n");
	quit(GOODEXIT);
}

void copymsg(FILE* f) {
	fprintf(f, nsc ?
"NLINK Version 1.3 by National Semiconductor Corp. (c) Copyright 1988\n" :
"ULINK Version 4.10 by Unidot Inc (c) Copyright 1982,1985,1987,1988\n");
	fprintf(f, " Resurrected 2026, AESilky\n");
}

/*
 * quit - Exits with the specified status, after closing files.
 */

void quit(int n){

	if( mflag && LIST ) {fclose( LIST ); LIST=(FILE*)0;}
	if (OBJOUT) { fclose(OBJOUT); OBJOUT = (FILE*)0; }
	if( LOCFILE ) {
		fclose( LOCFILE ); 
		LOCFILE=(FILE*)0;
		unlink(locfname); 
		locfname[0] = 0;
	}
	fclose( stdout );
	if (SYMFILE) {
		unlink(symfname);
		symfname[0] = 0;
	}
	if (RELFILE) {
		unlink(relfname);
		relfname[0] = 0;
	}
	if( (n == BADEXIT || n == FATEXIT) && objfile ){
		if( verbose ) printf("removing %s\n",objfile);
		unlink( objfile );
		objfile = (char*)0;
	}
#ifdef STATS
	if( verbose || debug || n == BADEXIT || n == FATEXIT ) prstats("at quit");
#endif
	exit(n);
}

#ifdef STATS
void prstats(char* s) {

	printf("memory stats %s  (sp = %x)\n",s,&s);
	printf("%6ld bytes used for buffers\n",usestats[BUFUSE]);
	printf("%6ld bytes used for sections\n",usestats[SECUSE]);
	printf("%6ld bytes used for symbols\n",usestats[SYMUSE]);
	printf("%6ld bytes used for groups\n",usestats[GRPUSE]);
	printf("%6ld bytes used for other items\n",usestats[OTHUSE]);
}
#endif

void flushclos(FILE* f) {
	fflush( f );
	if( ferror(f) ) error("F93object file write error");
	fclose( f );
}
