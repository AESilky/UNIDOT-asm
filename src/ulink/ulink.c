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

char	*symfname;
char	*relfname;
char	*locfname;
short	filex;
short	singlecol;
short	nsc;
short	wrnex = WRNEXIT;
char	nil[2];			/* empty string	*/
char	*files[LNKCNT];
long	curtime;
char	datstr[32];
char	date[16];
char	timstr[16];


intr(){  fprintf(stderr,"INTERRUPT!\n"); quit( FATEXIT ); }
/*		Here starts ulink			*/


main( argc, argv ) int argc; char *argv[];{

	reg SECTION	*sep;

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
			locfname = "ulcXXXXX";
			mktemp(locfname);
			LOCFILE = fopen( locfname, "w" );
			if( LOCFILE == NULL )
				nocreat( "local symbol file" );
#ifdef msdos
			setmode(fileno(LOCFILE),"text");
#endif
		}
	} else
		nflag = 0;
	if( afmt ){
		if( !sflag ){
			symfname = "usyXXXXX";
			mktemp(symfname);
			SYMFILE = fopen( symfname, "w" );
			if( SYMFILE == NULL ) nocreat("symbol file");
		}
		if( rflag ){
			relfname = "urlXXXXX";
			mktemp(relfname);
			RELFILE = fopen( relfname, "w" );
			if( RELFILE == NULL ) nocreat("relocation file");
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
	sectab[URBABS] = sep = (SECTION *)zpalloc( sizeof(SECTION), SECUSE );
#else
	sectab[URBABS] = sep = (SECTION *)zpalloc( sizeof(SECTION) );
#endif
	sep->se_sym = sylook( ".abs  " );		/* NOTE BLANKs	*/
	sep->se_sym->sy_typ = STSEC;			/* no list	*/
	sep->se_atr |= USEABS;				/* absolute	*/
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
		if( verbose ) printf("pass2:\n");
		dopass( filex, files );
		if( afmt ){
			afinish();
		} else {
			objblk.ob_type = UOBOND;
			oflush();
		}
		if( verbose ) printf("end of linking\n");
	}
	flushclos( OBJOUT );
	if( errct && verbose ) printf("pass 2 not performed\n");
	if( mflag ) map();
	if( ovct ) printf( "%4d overlapping sections\n", ovct );
	if( undct ) printf( "%4d undefined symbols\n", undct );
	if( mulct ) printf( "%4d multiply defined symbols\n", mulct );
	if( errct ){
		printf( "%4d linker errors\n", errct );
		quit( BADEXIT );
	}
	if( verbose ) printf( warnct ?
			"%4d linker warnings\n" :
			"no linker errors detected\n", warnct);
	quit( warnct ? wrnex : GOODEXIT );
}
/*
 * init - Initialization processing.
 */

init( argc, argv ) int argc; char **argv;{

	reg char	*ap;
	reg		i;

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
		case 'b': singlecol++;		/* single column map	*/
			  continue;

		case '0': if( *ap != 'd' ) usage();
			  while( *ap++ == 'd' ) debug++;
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
		error("F94 The -s and -r options may not be used together");
}

linkspec( lnkfile ) char *lnkfile; {	/* do a link file */

	reg char	*ap;
	reg		i;
	reg		ax;
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
	flushclos( LNKFILE );
	init( ax, aav );		/* recurse */
}
/*

 * scanaddr - convert an address
 */

long
scanaddr( s ) reg char *s; {

	long	v;
	reg int	c;

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
 * locspec - Processes a location specification string.
 */

locspec( s ) reg char *s;{


	reg char	*n;
	reg SECTION	*sep;
	char		name[128];

	n = name;
	while( *s != '\0' && *s != '=' ) *n++ = *s++;	/* copy the name */
	*n = name[32] = NULLCA;
	sep = selook( name );
	if( *s == '=' ){				/* read the address */
		sep->se_val = scanaddr( s+1 );
		sep->se_cum = 0L;
		sep->se_mod = 0L;
		sep->se_atr |= USEFIX;
	}
}
/*
 * defsym - Processes a symbol definition
 */

defsym( s ) reg char *s;{


	reg char	*n;
	reg SYTAB	*syp;
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

SYTAB *
rev(p) reg SYTAB *p; {
	reg SYTAB	*q;
	reg SYTAB	*r;

	q = 0;			/* last element on chain */
	while( p ) r = p->sy_lnk, p->sy_lnk = q, q = p, p = r;
	return q;
}

static char *symfmt;

pent(syp) reg SYTAB *syp; {	/* print a symbol entry	*/

	reg char	*valfmt;

	fprintf( LIST, symfmt, syp->sy_str );
	if( OVLYFILE ) fprintf( LIST, "%4d", syp->sy_ovl & 0xff );
	valfmt = " %8lx  ";
	if( syp->sy_typ == STUND ) valfmt =  "undefined  "; else
	if( syp->sy_atr & SAMUD )  valfmt =  " multiple  ";
	fprintf( LIST, valfmt, syp->sy_val );
}

psect(sep) reg SECTION *sep; {	/* print a section entry	*/

	reg SYTAB	*syp;
	reg SECTION	*sep2;
	int		i = 0;

#define SE_PRINTED 1
	if( sep->se_atr2 & SE_PRINTED ) return;
top:	sep->se_atr2 |= SE_PRINTED;
	if( linect < 1 ) page();
	syp = sep->se_sym;
	if( i ) fprintf( LIST, "%-25.25s ext %2d",syp->sy_str,i);
	   else fprintf( LIST, "%-32.32s", syp->sy_str);
	fprintf( LIST," %8lx %8lx %8lx        %2d         %2d        %2d\n",
		sep->se_val,
		sep->se_val+sep->se_cum,
		sep->se_cum,
		sep->se_aln,
		sep->se_ext,
		sep->se_adu);
	linect--;
	if( sep->se_atr & USEXTD1 ){
		i++;
		sep = sectab[sep->se_xtd & 0xff];
		goto top;
	}
}

bigspace(n){		/* output big space between sections */

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

map(){

	reg SYTAB	*syp;
	reg int		i;
	reg SYTAB	*syp2;
	SYTAB		*c1,*c2,*c3;	/* columns for printing */
	reg SECTION	*sep;
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
			sep = sectab[i];
			if( sep->se_atr & USEABS ) continue;
			if( sep->se_atr & USENOX ){
				if( sep->se_val < datsep->se_val )
					datsep->se_val = sep->se_val;
			} else {
				if( sep->se_val < codsep->se_val )
					codsep->se_val = sep->se_val;
			}
		}
		codsep->se_cum -= codsep->se_val;
		datsep->se_cum -= datsep->se_val;
		psect( codsep );
		linect -= 2;
		fprintf(LIST,"Sections comprising derived code section\n\n");
		for( i = URBSEC; i<stct; i++ ){
			sep = sectab[i];
			if( sep->se_atr & (USEABS|USENOX) ) continue;
			psect( sep );
		}
		bigspace(8);
		fprintf(LIST,"Derived data section for split code/data\n\n");
		psect( datsep );
		linect -= 2;
		fprintf(LIST,"Sections comprising derived data section\n\n");
		for( i = URBSEC; i<stct; i++ ){
			sep = sectab[i];
			if( sep->se_atr & USEABS ) continue;
			if( !(sep->se_atr & USENOX) ) continue;
			psect( sep );
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
		flushclos( LOCFILE );
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

page(){

	fprintf( LIST,
		"\f\n\n%-16s    %14s  %8s%-*sPage%4d\n\n%s\n\n",
			prname,date,timstr,28, " ", ++pagect, pghead );
	linect = 56;
}

afinish(){		/* clean up the a.out format module */

	reg int		i;

DEB(0,("afin fpos = %ld, relsize = %ld, symsize = %ld\n",fpos,relsize,symsize));
	fseek( OBJOUT, fpos, 0 );	/* move to end of file */
	fclean( fpos, 0 );
	if( rflag ){			/* copy the relocation items */
		flushclos( RELFILE );
		RELFILE = fopen( relfname, "r" );
		if( RELFILE == NULL ) noread("relocation file");
		while( (i = getc(RELFILE)) != EOF ) putc( i, OBJOUT );
		fclose( RELFILE );
	}
	if( !sflag ){			/* copy the symbols */
		flushclos( SYMFILE );
		SYMFILE = fopen( symfname, "r" );
		if( SYMFILE == NULL ) noread("symbol file");
		while( (i = getc(SYMFILE)) != EOF ) putc( i, OBJOUT );
		fclose( SYMFILE );
	}
	if( relsize || symsize || tranad ){
		fseek( OBJOUT, 4L, 0 );
		along( tranad,  OBJOUT );
		along( relsize, OBJOUT );
		along( symsize, OBJOUT );
	}
}

fclean( pos, length ) long pos;{

	static long hiwater = 0;

	if( hiwater < pos ){
		fseek( OBJOUT, hiwater, 0 );
		while( hiwater < pos ){
			putc( 0, OBJOUT );
			hiwater++;
		}
	}
	if( pos + length > hiwater ){
		hiwater = pos + length;
		DEB(0,("fclean: %ld + %d = %ld\n",pos,length,hiwater));
	}
}
/*
 * usage - Issues a fatal error message for a command line error.
 */

usage(){

	copymsg(stdout);
	printf("Usage: %s [options].. file [file]..\n",prname);
	printf("	-a		produce image output in a.out style\n");
	if( !nsc )
	printf("	-b		single column map file for symbols\n");
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
	quit(GOODEXIT);
}

copymsg(f)FILE *f;{
	fprintf(f, nsc ?
"NLINK Version 1.3 by National Semiconductor Corp. (c) Copyright 1988\n" :
"ULINK Version 4.10 by Unidot Inc (c) Copyright 1982,1985,1987,1988\n");
}

/*
 * quit - Exits with the specified status, after closing files.
 */

quit(n){

	if( mflag ) fclose( LIST );
	if( LOCFILE ) fclose( LOCFILE );
	if( OBJOUT ) fclose( OBJOUT );
	fclose( stdout );
	if( symfname ) unlink( symfname );
	if( relfname ) unlink( relfname );
	if( locfname ) unlink( locfname );
	if( (n == BADEXIT || n == FATEXIT) && objfile ){
		if( verbose ) printf("removing %s\n",objfile);
		unlink( objfile );
	}
#ifdef STATS
	if( verbose || n == BADEXIT || n == FATEXIT ) prstats("at quit");
#endif
	exit(n);
}

#ifdef STATS
prstats(s) char *s;{ 

	printf("memory stats %s  (sp = %x)\n",s,&s);
	printf("%6ld bytes used for buffers\n",usestats[BUFUSE]);
	printf("%6ld bytes used for sections\n",usestats[SECUSE]);
	printf("%6ld bytes used for symbols\n",usestats[SYMUSE]);
	printf("%6ld bytes used for groups\n",usestats[GRPUSE]);
	printf("%6ld bytes used for other items\n",usestats[OTHUSE]);
}
#endif

flushclos(f)FILE *f;{
	fflush( f );
	if( ferror(f) ) error("F93object file write error");
	fclose( f );
}
