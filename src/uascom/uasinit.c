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
*		uas.init.c - init code and miscellaneous		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasinit.c,v 6.15 88/11/20 13:27:45 rmm Rel $ uas initialization";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif

#ifdef MSC
#include <fcntl.h>
int	_fmode = O_BINARY;
#else
#include <fcntl.h> /* For 'open' */
#include <unistd.h> /* For 'close' and `sbrk` */
int	_iomode = 1;
#endif
#include "funcdefs.h"		/* Forward defines for GCC */

#include <stdarg.h>
#include <string.h>
#include <time.h>

int argnum(char* s);
void defsym(char* s);
void usage(char* s, ...);

char		objsuf[8] = "obj";	/* predef may change this */
char		lstsuf[8] = "lst";	/* predef may change this */
char		srcsuf[8] = {NULLCA};	/* predef may change this */
char		*pdpath[] = { "./u", "/lib/u", "/usr/lib/u", 0 };
char		*lstname = 0;		/* listing name		  */

/*
 * Variables for I/O
 */


#ifndef NOPD
/*
 * badpre - Issues a fatal error for a bad PREDEF file.
 */

badpre(){

	fatal( "72 PREDEF file error at line %u", infp-> in_seq );
}
#endif

/*
 * getdat - Gets the date and time and puts them into datstr.
 */

void getdat(){

	long	curtime;

	time(&curtime );
	strcpy( datstr, ctime(&curtime ));
	strncpy( timstr, datstr+11, 8);
	strncpy( date, datstr+4, 7 );
	strncpy( date+7, datstr+20, 4 );
}

char *
lastcomp(char* s) {	/* find last component of file name */

	char *q;

	/* following code works well for UNIX, MSDOS, and VMS */

	for( q = s; *s; s++ )
		if( *s == '/' || *s == '\\' || *s == ':' || *s == ']' )
			q = s+1;
	return q;
}
/*
 * init - Performs assembler initialization.
 */
void init(int argc, char* argv[]) {


	char		**av;
	char		*ap;
	char		*ep;
	char		*sp;
	int		fd;
	int		i;
	int		argn;
	int		j;
	int		defx;
	short		nerrlim;
	short		sufsiz;
	char		*objname;
	char		*errfile;
	char		fnbuf[24];
	char		*defines[32];
#ifndef NOPD
	char		pdname[64];
#endif
	char		*asmsuf;
	//extern char	*sbrk();
	static char	nil[2];

	objname = 0;
	nerrlim = 0;
	errfile = 0;
	defx = 0;
	prname = lastcomp( argv[0] );
	if( strlen( prname ) > 10 ) prname[10] = NULLCA;
	if( argc < 2 ) usage((char *)0);
	phytop = phylim = sbrk( 0 );
	blklog = 1;
	while( (1<<blklog) < BUFSIZ ) blklog++;
	if( BUFSIZ != (1<<blklog) )
		fatal("51 BUFSIZ=%d, blklog =%d",BUFSIZ,blklog);
	if( sizeof(VMADR) < sizeof(char *) )
		fatal("60 VMADR = %d bytes, char * = %d bytes",
			sizeof(VMADR),sizeof(char *));
	getdat();

	/* find the string after the first 's' or 'S' in the assembler name */

#ifndef NOPD
	for( asmsuf = prname;
	     *asmsuf && *asmsuf != 's' && *asmsuf != 'S';
	     asmsuf++ );
	if( *asmsuf ) asmsuf++; else asmsuf = prname;
#endif

	argn = 1;
top:	while( argn < argc ){	/* read command line arguments */
		ap = argv[argn++];
		if( *ap == '-' ){		/* switches		*/
			ap++;			/* skip the '-'		*/
			for(;;)switch( *ap++ ){

		case 0:		goto top;
		case 'a':
		case 'A':	if( strcmp(prname,"nrgpasm") &&
				    strcmp(prname,"NRGPASM")) break;
				aopt = 1;		continue;

		case 'D':
		case 'd':		/* define symbol */
				if( defx >= 32 ) fatal("90 defx oflo");
				if( *ap ){
					defines[defx++] = ap;
				} else {
					if( argn >= argc )
						usage("no definition");
					// In this case (sp between -d and sym=val)
					// remove any '\' characters
					defines[defx] = argv[argn++];
					i = defx++;
					char *s = defines[i];
					int bs = 0;
					int sl = 0;
					while(*s) {
						if (*s++ == '\\')
							bs = 1;
						else
							sl++;
					}
					if (bs) {
						// There are '\' characters than need to be removed
						s = defines[i];
						char *d = palloc(sl+1);
						defines[i] = d;
						while(*s) {
							if (*s != '\\')
								*d++ = *s++;
							else
								s++;
						}
						*d = NULLCA;
					}
				}
				goto top;

		case 'E':
		case 'e':		/* set error limit	*/
				if( *ap ){
					if( *ap == '=' ) ap++;
					i = argnum(ap);
				} else {
					if( argn >= argc )
						usage("no error limit");
					i = argnum(argv[argn++]);
				}
				if( i == 0 ) i = 32767;
				nerrlim = i;
				goto top;

		case 'F':		/* set the list file name	*/
		case 'f':	if( lstname ) usage( "too many list files");
				if( *ap ){
					if( *ap == '=' ) ap++;
					lstname = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no object file name");
				lstname = argv[argn++];
				goto top;

		case 'H':
		case 'h':		/* page height in lines */
				if( *ap ){
					if( *ap == '=' ) ap++;
					i = argnum(ap);
				} else {
					if( argn >= argc )
						usage("no page height");
					i = argnum(argv[argn++]);
				}
				if( i < 9 || i > 106 )
					usage("height not 9-106: %d\n",i);
				llpp = i - 8;	/* room for header	*/
				goto top;
		case 'I':
		case 'i':		/* include paths	*/
				if( *ap ){
					if( *ap == '=' ) ap++;
					inclpath[inclx++] = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no object file name");
				inclpath[inclx++] = argv[argn++];
				goto top;

		case 'L':
		case 'l':	lflag = 1;		continue;

#ifdef ONECASE
		case 'M':
		case 'm':	upperonly ^= 1; continue; /* map to uc */
#endif

		case 'N':
		case 'n':	if( objname ) usage("both -n and -o specified");
				noobj = 1;
				continue;

		case 'O':
		case 'o':	if( objname ) usage( "too many object files");
				if( noobj ) usage("both -n and -o specified");
				if( *ap ){
					if( *ap == '=' ) ap++;
					objname = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no object file name");
				objname = argv[argn++];
				goto top;

#ifndef NOPD
		case 'P':
		case 'p':	if( *ap ){
					if( *ap == '=' ) ap++;
					strcpy( pdname, ap );
				} else
					strcpy( pdname, "." );
				goto top;
#endif

		case 'Q':
		case 'q':	quiet++;
				if (argn >= argc) usage((char*)0);
				continue;

		case 'S':
		case 's':	if( *ap ){
					if( *ap == '=' ) ap++;
					dfltsec = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no section specified");
				dfltsec = argv[argn++];
				goto top;

#ifdef OLDNSCCODE
		case 'T':
		case 't':	if( *ap ){
					if( *ap == '=' ) ap++;
					sysparm = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no sysparm specified");
				sysparm = argv[argn++];
				goto top;
#endif

		case 'U':
		case 'u':	uext = 1;
				continue;

		case 'V':
		case 'v':	verbose++;
				if( argn >= argc ) usage( (char *)0 );
				continue;

		case 'W':
		case 'w':		/* page width in chars */
				if( *ap ){
					if( *ap == '=' ) ap++;
					i = argnum(ap);
				} else {
					if( argn >= argc )
						usage("no page width");
					i = argnum(argv[argn++]);
				}
				if( i < 40 || i > 210 )
					usage("width not 40-210: %d\n",i);
				slbr = &sline[i];
				if( i ) i -= 24;	/* move over */
				rmarg = pgwd = i;
				goto top;

		case 'X':
		case 'x':	xflag++;
				lflag = 1;	continue;

		case '0':	while( *ap == 'd' ) debug++, ap++;
#ifdef	STATS
				if( *sp == 'z' ) stats = 1;
#endif
				verbose++;
				continue;

		case 'Z':
		case 'z':	if( errfile ) usage( "too many error files");
				if( *ap ){
					if( *ap == '=' ) ap++;
					errfile = ap;
					goto top;
				}
				if( argn >= argc )
					usage("no error file name");
				errfile = argv[argn++];
				goto top;

		default:	usage("unrecognized option: %s",ap);
			}
		}

		/* file name */

		if( srcfile[0] ) usage("too many source files");
		strcpy( srcfile, ap );
	}
	if( !srcfile[0] ) usage("no sourcefile");
	sp = lastcomp( srcfile );
	if( (j = strlen(sp)) > 14 ) sp[j = 14] = NULLCA;
	strcpy( titl1, srcfile );
	for( i = -1, ep = sp; *ep; ep++ )
		if( *ep == '.' ){
			if( i != -1 ) fatal("65 File name has too many '.'s");
			i = ep-sp;
		}
	if( i != -1 ){
		if( i == 0 ) fatal("64 File name has no prefix: %s",sp);
		ep = sp+i;
		if( !ep[1] )fatal("66 File name has zero length suffix: %s",sp);
	}
#ifdef msdos
	if( ep-sp > 8 ){	/* base name too long	*/
		strcpy( sp+8, ep );		/* move it down		*/
		ep = sp+8;
	}
	if( strlen(ep) > 4 ) ep[4] = NULLCA;		/* trim extension	*/
#endif
#ifndef NOPD
	for( j=0; pdpath[j]; i++ ){
		strcpy( pdname, pdpath[j] );
		strcat( pdname, asmsuf );
		strcat( pdname, ep );
		strcat( pdname, ".pd" );
		if( include(pdname) != -1 ) break;
	}
	if( pdpath[j] == 0 ) fatal( "69 No PREDEF file (%s)", pdname );
#endif
	urabyte = URAA8;		/* default relocation actions	*/
	uralong = URAA32;		/* default relocation actions	*/
	uraword	= URAA16;		/* default relocation actions	*/
	predef();
	if( xflag > 1 ) xsline = !xsline;	/* toggle flag		*/
	if( xsline && pgwd >= 16 ) pgwd -= 3;
	if( nerrlim ) errlim = nerrlim;
	if( srcsuf[0] == 0 && *ep ) strcpy( srcsuf, ep+1 );
	sufsiz = strlen(srcsuf);
	if( sufsiz > 3 ) srcsuf[sufsiz=3] = NULLCA;
	i = strlen(lstsuf);
	if( i > 3 ) lstsuf[i=3] = NULLCA;
	if( i > sufsiz ) sufsiz = i;
	i = strlen( objsuf );
	if( i > 3 ) objsuf[i=3] = NULLCA;
	if( i > sufsiz ) sufsiz = i;
	if( strcmp(srcsuf,lstsuf) == 0 || strcmp(srcsuf,objsuf) == 0 )
		fatal( "84 Illegal suffix for source: .%s\n",srcsuf);
	fd = open(srcfile, 0);
	if( fd == -1 && srcsuf[0] == 0 ) fatal( "54 Cannot open %s", srcfile );
	if( *ep == 0 ){
		if( sufreqd ) fatal("73 Source must have suffix '%s'",srcsuf);
#ifndef msdos
		if( ep-sp > 13-sufsiz ) ep = sp + 13-sufsiz;
#endif
		*ep = '.';
		strcpy( ep+1, srcsuf );
	} else
	if( sufreqd && strcmp(srcsuf,ep+1) )
	    fatal("74 Source suffix is '%s', but should be '%s'",ep+1,srcsuf);
	if( fd == -1 ) fd = open(srcfile, 0);
	if( fd == -1 ) fatal( "54 Cannot open %s", srcfile );
	close( fd );
	strcpy( fnbuf, sp );
	ep = fnbuf + (ep-sp);
	sp = fnbuf;
	strcpy( ep+1, objsuf );
	if( objname == 0 ) objname = sp;
	if( !noobj ){
		OBJECT = fopen( objname, "w" );
		if( OBJECT == NULL ) fatal( "52 Cannot create %s", objname );
		setbuf( OBJECT, palloc(BUFSIZ) );
	}
	if( lflag ){
		LIST = stdout;
		if( !lstname || strcmp( lstname, "-" ) ){
			strcpy( ep+1, lstsuf );
			if( lstname == 0 ) lstname = sp;
#ifdef MSC
			_fmode = O_TEXT;
#else
			_iomode = 0;
#endif
			LIST = fopen( lstname, "w" );
#ifdef MSC
			_fmode = O_BINARY;
#else
			_iomode = 1;
#endif
			if( LIST == NULL )
				fatal( "52 Cannot create %s", lstname );
		}
		setbuf( LIST, palloc(BUFSIZ) );
	}
	for( i=0; i<defx; i++ ) defsym( defines[i] );
	if( errfile ){
		ERRFIL = stdout;
		if( strcmp( errfile, "-" ) ){
#ifdef MSC
			_fmode = O_TEXT;
#else
			_iomode = 0;
#endif
			ERRFIL = fopen( errfile, "w" );
#ifdef MSC
			_fmode = O_BINARY;
#else
			_iomode = 1;
#endif
			if( ERRFIL == NULL ){
				ERRFIL = stderr;
				fatal( "52 Cannot create %s", errfile );
			}
		}
		setbuf( ERRFIL, palloc(BUFSIZ) );
		fprintf(ERRFIL,"error listing file for %s %s %s\n",
			srcfile,date,timstr);
	}
}

int argnum(char* s) {

	int	i;

	i = 0;
	while( *s ){
		if( *s < '0' || *s > '9' ) return 0;
		i = i * 10 + *s++ - '0';
	}
	return i;
}

void defsym(char* s) {

	char	*p;
	sytab_t	*syp;
	int		i;
	VMADR		val;
	char	defbuf[64];

	p = defbuf;
	while( *s && *s != '=' ){
		i = *s++;
		if( upperonly && i >= 'a' && i <= 'z' ) i += 'A' - 'a';
		*p++ = i;
	}
	*p = 0;
	if( *s == '=' ) s++; else s = "1";
	if( defbuf[0] == 0 ) usage("no define symbol");
	scanpt = s;
	i = token();
	if( i != TKCON && i != TKSTR ) usage("illegal define value");
	val = sylook(defbuf);
	syp = (sytab_t *)wfetch(val);
	if( syp->sy_typ != STUND ) usage("symbol %s previous defined",defbuf);
	syp->sy_typ = STVAR;
	syp->sy_val = SYVAL(tokval);
	syp->sy_atr = SADP2;
	if( xflag ){
		pass2++;
		xref( val, XRDEF );
		pass2--;
	}
}
/*
 * preget - Gets a token, issues a fatal error if it is not the specified
 * type, and returns the token's value.
 */

#ifndef NOPD
preget(int typ) {

	if( token() != typ ) badpre();
	return tokval;
}
#endif

void copymsg(FILE* f) {
	fprintf(f, "UAS Macro Assembler Version 6.18 by Unidot Inc (c) Copyright 1982,1985,1987,1988\n");
	fprintf(f, " Resurrected 2026, AESilky\n");
}


/*
 * usage - Issues a fatal error for an illegal command line.
 */

void usage(char* s, ...) {
	if( verbose ) predef();
	if( s ){
		va_list argptr;
		va_start(argptr, s);
		fprintf(ERRFIL,"Fatal:	");
		vfprintf(ERRFIL,s,argptr);
		fprintf(ERRFIL,"\n");
		va_end(argptr);
	}
	copymsg(ERRFIL);
	fprintf(ERRFIL, "Usage:  %s [options]... file\n", prname);
	if( !strcmp(prname,"nrgpasm") || !strcmp(prname,"NRGPASM") )
		fprintf(ERRFIL,"\t-a              absolute addressing\n");
	fprintf(ERRFIL,"\t-d<sym>=<val>   define a symbol value (raw)\n");
	fprintf(ERRFIL, "\t-d <sym>=<val> define a symbol value (removes '\\')\n");
	fprintf(ERRFIL,"\t-e<nnn>         set an error limit\n");
	fprintf(ERRFIL,"\t-f<lstname>     change list file name from default to <lstname>\n");
	fprintf(ERRFIL,"\t-h<nnn>         set page height in lines\n");
	fprintf(ERRFIL,"\t-i<path>        set an include path\n");
	fprintf(ERRFIL,"\t-l              produce a listing file\n");
	fprintf(ERRFIL,"\t-m              toggle the uppercase only option\n");
	fprintf(ERRFIL,"\t-n              do not produce an object file\n");
	fprintf(ERRFIL,"\t-o<objname>     change object name from default to <objname>\n");
#ifndef NOPD
	fprintf(ERRFIL,"\t-p<pdfile>    set a predefinition file\n");
#endif
	fprintf(ERRFIL,"\t-q            be quiet (except for errors)\n");
	fprintf(ERRFIL,"\t-s<sectname>  set a default section name\n");
#ifdef OLDNSCCODE
	fprintf(ERRFIL,"\t-t<sysparm>   set a system parameter\n");
#endif
	fprintf(ERRFIL,"\t-u            make all undefineds external\n");
	fprintf(ERRFIL,"\t-v            be more verbose than usual\n");
	fprintf(ERRFIL,"\t-w<nnn>       set page width in characters\n");
	fprintf(ERRFIL,"\t-x            collect cross references\n");
	fprintf(ERRFIL,"\t-xx           alternate cross reference list\n");
	fprintf(ERRFIL,"\t-z<errfile>   errors to <errfile> and not stderr\n");
	fprintf(ERRFIL,"\t-0d[d...]     output debug messages (more d's more output)\n");
	quit( BADEXIT );
}
